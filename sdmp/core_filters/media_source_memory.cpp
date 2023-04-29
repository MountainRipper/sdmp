#include "media_source_memory.h"

namespace mr::sdmp{

COM_REGISTER_OBJECT(MediaSourceMemoryFilter)

MediaSourceMemoryFilter::MediaSourceMemoryFilter()
{    
    properties_["pushGrabber"] = (void*)static_cast<IFilterHandlerDataGrabber*>(this);
}
int32_t MediaSourceMemoryFilter::initialize(IGraph *graph, const Value &filter_values)
{
    //add a default pin for accept first stream connect
    create_general_pin(AVMEDIA_TYPE_UNKNOWN,kOutputPin);
    GeneralFilter::initialize(graph,filter_values);
    return 0;
}

int32_t MediaSourceMemoryFilter::connect_match_input_format(IPin *sender_pin,IPin *input_pin)
{
    (void)sender_pin;
    (void)input_pin;
    return 0;
}

int32_t MediaSourceMemoryFilter::connect_chose_output_format(IPin *output_pin, int32_t index)
{
    (void)output_pin;
    (void)index;
    return 0;
}

int32_t MediaSourceMemoryFilter::receive(IPin *input_pin, FramePointer frame)
{

    return 0;
}

int32_t MediaSourceMemoryFilter::requare(int32_t duration, const std::vector<PinIndex> &output_pins)
{
    frames_.pop_each([this](std::shared_ptr<sdmp::Frame> frame){
        this->get_pin(kOutputPin,0)->deliver(frame);
    });
    return 0;
}

int32_t MediaSourceMemoryFilter::process_command(const std::string &command, const Value &param)
{
    GeneralFilter::process_command(command,param);
    if(command == kGraphCommandStop || command == kGraphCommandPlay){
        frames_.clear();
    }
    return 0;
}

int32_t MediaSourceMemoryFilter::grabber_get_format(const std::string &id, const Format *format)
{
    (void)id;
    output_format_ = *format;

    if(output_format_.type == AVMEDIA_TYPE_AUDIO){
        if(output_format_.codec >= AV_CODEC_ID_FIRST_AUDIO && output_format_.codec < AV_CODEC_ID_PCM_VIDC){
            int bytes_per_second = 0;
            av_samples_get_buffer_size(&bytes_per_second,output_format_.channels,output_format_.samplerate,(AVSampleFormat)output_format_.format,1);
            output_format_.bitrate = bytes_per_second * 8;
        }
    }
    else if(output_format_.type == AVMEDIA_TYPE_VIDEO){
        if(output_format_.codec == AV_CODEC_ID_RAWVIDEO){
            output_format_.bitrate = av_image_get_buffer_size((AVPixelFormat)output_format_.format,output_format_.width,output_format_.height,1)*8*output_format_.fps;
        }
    }
    sync_update_pin_format(kOutputPin,0,0,output_format_);
    return 0;
}

int32_t MediaSourceMemoryFilter::grabber_get_frame(const std::string &id, FramePointer frame)
{
    if(status_ != kStatusRunning)
        return 0;
    auto av_frame = frame->frame;
    auto av_packet = frame->packet;
    if(av_frame){
        if(output_format_.format != av_frame->format ||
            output_format_.width != av_frame->width ||
            output_format_.height != av_frame->height||
            output_format_.samplerate != av_frame->sample_rate ||
            output_format_.channels != av_frame->ch_layout.nb_channels){
            MR_ERROR("MediaSourceMemoryFilter::grabber_get_frame get frame not match format set");
            return kErrorConnectFailedNotMatchFormat;
        }
    }
    else if(!av_packet){
        // without both frame and packet
        return kErrorInvalidParameter;
    }

    frames_.push(frame);
    return 0;
}


}
