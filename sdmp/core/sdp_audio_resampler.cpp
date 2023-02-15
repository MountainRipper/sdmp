#include "sdp_audio_resampler.h"
namespace mr::sdmp {


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
    av_channel_layout_default(&channel_layout_,channels_);
    return 0;
}

int32_t SdpAudioResampler::push_audio_samples(int32_t samplerate, int32_t channels, AVSampleFormat format, int32_t samples, uint8_t *pcm[])
{

    AVFrame audio_frame_in;
    audio_frame_in.sample_rate = samplerate;
    audio_frame_in.nb_samples = samples;
    audio_frame_in.format = format;
    av_channel_layout_default(&audio_frame_in.ch_layout,channels);
    int32_t linesize = av_get_bytes_per_sample(format) * samples;
    if(av_sample_fmt_is_planar(format)){
        audio_frame_in.extended_data = audio_frame_in.data;
        for(int index = 0; index < channels; index++){
            audio_frame_in.data[index] = pcm[index];
            audio_frame_in.linesize[index] = linesize;
        }
    }
    else{
        audio_frame_in.data[0] = pcm[0];
        audio_frame_in.linesize[0] = linesize * channels;
    }

    return push_audio_samples(&audio_frame_in);
}

int32_t SdpAudioResampler::push_audio_samples(const AVFrame *sample)
{
    int32_t samplerate = sample->sample_rate;
    int32_t channels = sample->ch_layout.nb_channels;
    AVSampleFormat format = (AVSampleFormat)sample->format;

    if( samplerate_in_ != samplerate ||
        channels_in_ != channels ||
        format_in_ != format)
    {
        need_create_resample_ = true;
    }

    int ret = 0;
    if(need_create_resample_)
    {
        av_channel_layout_default(&channel_layout_in_,channels_in_);
        resampler_ = swr_alloc();
        ret = swr_alloc_set_opts2(&resampler_,
                            &channel_layout_,   // out_ch_layout
                            format_,            // out_sample_fmt
                            samplerate_,        // out_sample_rate
                            &channel_layout_in_,// in_ch_layout
                            format_in_,         // in_sample_fmt
                            samplerate_in_,     // in_sample_rate
                            0,                  // log_offset
                            NULL);              // log_ctx
        if(ret < 0){
            return ret;
        }
        samplerate_in_ = samplerate;
        channels_in_   = channels;
        format_in_     = format;
        need_create_resample_ = false;
    }
    if(!swr_is_initialized(resampler_))
        return -2;

    ret = swr_convert_frame(resampler_,nullptr,sample);
    if(ret < 0)
        return ret;

    return 0;
}

FramePointer SdpAudioResampler::pull(int32_t samples)
{
    FramePointer frame;
    if(samples == 0)
        return frame;

    if(!resampler_)
        return frame;

    if(!swr_is_initialized(resampler_))
        return frame;


    int32_t samples_has = swr_get_delay(resampler_,samplerate_);
    if(samples_has == 0)
        return frame;
    if(samples > samples_has){
        samples = samples_has;
    }
    AVFrame* av_frame = av_frame_alloc();
    av_frame->format = format_;
    av_frame->nb_samples = samples;
    av_frame->ch_layout = channel_layout_;
    swr_convert_frame(resampler_,av_frame,nullptr);
    frame = Frame::make_frame(av_frame);
    frame->releaser = sdp_frame_free_frame_releaser;
    return frame;
}


int32_t SdpAudioResampler::samples()
{
    if(!resampler_)
        return -1;
    if(!swr_is_initialized(resampler_))
        return -2;
    return swr_get_delay(resampler_,samplerate_);
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
