#include "audio_resample_mixer.h"
#include <tio/tio_software_frame.h>

namespace mr::sdmp {

COM_REGISTER_OBJECT(AudioResampleMixer)


AudioResampleMixer::AudioResampleMixer()
{

}


int32_t sdmp::AudioResampleMixer::initialize(IGraph *graph, const Value &filter_values)
{
    create_general_pin(AVMEDIA_TYPE_AUDIO,kInputPin);
    create_general_pin(AVMEDIA_TYPE_AUDIO,kOutputPin);

    auto ret = GeneralFilter::initialize(graph,filter_values);

    resamplers_.push_back(std::make_shared<SdpAudioResampler>());

    refresh_property();

    return ret;
}


int32_t AudioResampleMixer::connect_match_input_format(IPin *sender_pin,IPin *input_pin)
{
    (void)input_pin;
    const auto& formats = sender_pin->formats();
    int index = 0;
    for (const auto& item :formats ) {
        if(item.type == AVMEDIA_TYPE_AUDIO && !av_sample_fmt_is_planar((AVSampleFormat)item.format) && item.channels > 0 && item.samplerate > 0){
            //set current input pin format
            sync_update_pin_format(kInputPin,input_pin->index(),0,item);
            break;
        }
        index++;
    }

    if(index >= formats.size()){
        return -1;
    }

    //create a new pin for others upstream filter to connect
    auto& pins = get_pins(kInputPin);
    bool has_unconnected = false;
    for(auto pin : pins){
        if(pin->sender() || pin->index() == input_pin->index())
            continue;
    }
    if(!has_unconnected){
        create_general_pin(AVMEDIA_TYPE_AUDIO,kInputPin);
        resamplers_.push_back(std::make_shared<SdpAudioResampler>());
        resamplers_.back()->reset(format_out_.samplerate, format_out_.channels, (AVSampleFormat)format_out_.format);
    }


    return index;
}

int32_t AudioResampleMixer::receive(IPin *input_pin, FramePointer frame)
{
    auto pin_index = input_pin->index();
    if(pin_index >= resamplers_.size())
        return kErrorResourceNotFound;

    auto& resampler = resamplers_[pin_index];

    if(frame->frame)
        resampler->push_audio_samples(frame->frame);

    if(frame->flag & kFrameFlagEos){
        resampler->push_audio_samples(nullptr);
    }

    return 0;
}

int32_t AudioResampleMixer::requare(int32_t duration, const std::vector<PinIndex> &output_pins)
{
    int32_t pull_duration = 0;
    for(auto& item : resamplers_){
        auto duration = item->duration();
        if(duration > pull_duration )
            pull_duration = duration;
    }

    pull_duration = std::min(pull_duration, duration);
    if(pull_duration <= 0)
        return duration;

    int samples = duration * format_out_.samplerate / 1000;

    AVFrame* av_frame = av_frame_alloc();
    auto dest = Frame::make_frame(av_frame);
    av_samples_alloc(av_frame->data, av_frame->linesize, format_out_.channels, samples , (AVSampleFormat)format_out_.format,1);
    dest->releaser = sdmp_frame_free_frame_releaser;

    auto pcm_mixer_dest = av_frame->data[0];

    auto bytes = av_frame->linesize[0];

    if(pcm_mixer_src_size_ < bytes){
        pcm_mixer_src_size_ = bytes + 128;
        pcm_mixer_src_ = sdmp::BufferUtils::create_shared_buffer(pcm_mixer_src_size_);
    }

    // std::lock_guard<std::mutex> lock(sender_mutex_);
    for(auto& item : resamplers_){
        memset(pcm_mixer_src_.get(),0,pcm_mixer_src_size_);
        auto frame = item->pull(samples);

        auto av_frame = frame->frame;
        if(av_frame){
            mixer_.input_stream((AudioMixer::Format)av_frame->format, av_frame->data[0], av_frame->nb_samples * format_out_.channels);
        }
    }

    mixer_.get_output((AudioMixer::Format)format_out_.format, pcm_mixer_dest, samples * format_out_.channels);

    (void)output_pins;
    return duration;
}

int32_t AudioResampleMixer::connect_chose_output_format(IPin *output_pin, int32_t index)
{
    (void)output_pin;
    (void)index;
    return 0;
}


int32_t AudioResampleMixer::property_changed(const std::string &property, Value &symbol)
{
    refresh_property();
    return 0;
}

void AudioResampleMixer::refresh_property()
{
    format_out_.channels = properties_["channels"];
    format_out_.samplerate = properties_["samplerate"];
    std::string fmt = properties_["format"];
    format_out_.format = (AVSampleFormat)av_get_sample_fmt(fmt.c_str());

    for(auto& item : resamplers_){
        item->reset(format_out_.samplerate, format_out_.channels, (AVSampleFormat)format_out_.format);
    }

    sync_update_pin_format(kOutputPin,0,0,format_out_);
}

int32_t AudioResampleMixer::disconnect_input(int32_t input_pin)
{
    //not need to release pins & resamplers,just keep it for next connect
    return 0;
}



};


