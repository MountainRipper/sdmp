#include "sdp_audio_resampler.h"
namespace sdp {


SdpAudioResampler::SdpAudioResampler(void)
{
	samplerate_ = 0;
	channels_ = 0;
	format_ = AV_SAMPLE_FMT_NONE;

	samplerate_in_ = 0;
	channels_in_ = 0;
	format_in_ = AV_SAMPLE_FMT_NONE;

	sample_bytes_ = 0;
    need_create_resample_ = false;
}

SdpAudioResampler::~SdpAudioResampler(void)
{
}

int32_t SdpAudioResampler::reset(int32_t samplerate, int32_t channels, AVSampleFormat format)
{
	if( samplerate_ == samplerate &&
		  channels_ == channels &&
		  format_ == format)
	{
		return 0;
	}
	samplerate_ = samplerate;
	channels_ = channels;
	format_ = format;
    sample_bytes_ = av_get_bytes_per_sample(format);

    return 0;
}

int32_t SdpAudioResampler::set_channel_mapping(const std::vector<int32_t> &mapping)
{
    need_create_resample_ = true;

    mapping_request_ = mapping;
    if(mapping_request_.size()){
        for(int32_t index = 0; index < channels_in_ ; index++){
            int channel_index = ((index >= mapping_request_.size()) ? index : mapping_request_[index]);
            mapping_index_[index] = (channel_index >= channels_in_) ? index : channel_index;
        }
    }
    return 0;
}

int32_t SdpAudioResampler::push_audio_samples(int32_t samplerate, int32_t channels, AVSampleFormat format, int32_t samples, uint8_t *pcm[])
{

    av::AudioSamples audio_frame_in;
    audio_frame_in.raw()->sample_rate = samplerate;
    audio_frame_in.raw()->channels = channels;
    audio_frame_in.raw()->nb_samples = samples;
    audio_frame_in.raw()->format = format;
    audio_frame_in.raw()->channel_layout = av_get_default_channel_layout(channels);
    int32_t linesize = av_get_bytes_per_sample(format) * samples;
    if(av_sample_fmt_is_planar(format)){
        audio_frame_in.raw()->extended_data = audio_frame_in.raw()->data;
        for(int index = 0; index < channels; index++){
            audio_frame_in.raw()->data[index] = pcm[index];
            audio_frame_in.raw()->linesize[index] = linesize;
        }
    }
    else{
        audio_frame_in.raw()->data[0] = pcm[0];
        audio_frame_in.raw()->linesize[0] = linesize * channels;
    }

    return push_audio_samples(audio_frame_in);
}

int32_t SdpAudioResampler::push_audio_samples(const av::AudioSamples &sample)
{
    int32_t samplerate = sample.sampleRate();
    int32_t channels = sample.channelsCount();
    AVSampleFormat format = sample.sampleFormat();

    if( samplerate_in_ != samplerate ||
        channels_in_ != channels ||
        format_in_ != format)
    {
        need_create_resample_ = true;
    }

    std::error_code error;

    if(need_create_resample_)
    {
        int *mapping = nullptr;
        if(mapping_request_.size()){
            mapping = mapping_index_;
        }
        resampler_ = av::AudioResampler();
        av::Dictionary dict;
        //dict.set("resampler",(int)SWR_ENGINE_SOXR);
        resampler_.init(av_get_default_channel_layout(channels_),samplerate_,format_,
                        av_get_default_channel_layout(channels),samplerate,format,
                        dict,mapping,
                        error);
        if(error)
            return error.value();
        samplerate_in_ = samplerate;
        channels_in_   = channels;
        format_in_     = format;
        need_create_resample_ = false;
    }
    if(resampler_.isValid() == false)
        return -2;

    resampler_.push(sample,error);
    if(error)
        return error.value();

    return 0;
}

av::AudioSamples SdpAudioResampler::pull(int32_t samples)
{
    if(samples == 0)
        return av::AudioSamples();

    if(resampler_.isValid() == false)
        return 0;

    if(samples > resampler_.delay()){
        samples = resampler_.delay();
    }

    std::error_code error;
    av::AudioSamples sample_output = resampler_.pop(samples,error);
    return sample_output;
}


int32_t SdpAudioResampler::samples()
{
    return resampler_.delay();
}
int32_t SdpAudioResampler::samplerate()
{
	return samplerate_;
}
int32_t SdpAudioResampler::channles()
{
	return channels_;
}
AVSampleFormat SdpAudioResampler::format()
{
	return format_;
}
int32_t SdpAudioResampler::sample_bytes()
{
    return sample_bytes_;
}

int32_t SdpAudioResampler::clear()
{
    pull(samples());
    return 0;
}

}
