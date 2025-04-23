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
            //create a new pin for others upstream filter to connect
            create_general_pin(AVMEDIA_TYPE_AUDIO,kInputPin);
            return index;
        }
        index++;
    }
    return -1;
}

int32_t AudioResampleMixer::receive(IPin *input_pin, FramePointer frame)
{
    (void)input_pin;

    if(frame->frame == nullptr)
        return kErrorInvalidFrame;

    if(frame->frame->width == 0 || frame->frame->height == 0)
        return kErrorInvalidFrame;

    if(av_tio_format_map.find((AVPixelFormat)frame->frame->format) == av_tio_format_map.end()){
        return kErrorInvalidFrame;
    }

    FramePointer new_frame = sdmp_frame_new(format_out_.format,format_out_.width,format_out_.height);
    auto tio_in = frame_to_tio(*frame->frame);
    auto tio_out = frame_to_tio(*new_frame->frame);
    SoftwareFrameConvert::convert(tio_in,tio_out,mr::tio::kRotate0,(FillMode)output_fillmode_);

    new_frame->frame->pts = frame->frame->pts;
    get_pin(kOutputPin,0)->deliver(new_frame);
    return 0;
}

int32_t AudioResampleMixer::requare(int32_t duration, const std::vector<PinIndex> &output_pins)
{
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

    sync_update_pin_format(kOutputPin,0,0,format_out_);
}

int32_t AudioResampleMixer::disconnect_input(int32_t input_pin)
{
    if(input_pin < resamplers_.size())
        resamplers_.erase(resamplers_.begin() + input_pin);
    return 0;
}



};


