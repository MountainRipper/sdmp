#include "video_frame_convert.h"
#include "textureio/src/convert_manager.h"
#include "tio/tio_software_frame.h"
namespace mr::sdmp {

COM_REGISTER_OBJECT(VideoFrameConvert)


VideoFrameConvert::VideoFrameConvert()
{

}


int32_t sdmp::VideoFrameConvert::initialize(IGraph *graph, const Value &filter_values)
{
    create_general_pin(AVMEDIA_TYPE_UNKNOWN,kInputPin);
    create_general_pin(AVMEDIA_TYPE_UNKNOWN,kOutputPin);

    auto ret = GeneralFilter::initialize(graph,filter_values);

    refresh_property();
    return ret;
}


int32_t VideoFrameConvert::connect_match_input_format(IPin *sender_pin,IPin *input_pin)
{
    (void)input_pin;
    const auto& formats = sender_pin->formats();
    int index = 0;
    for (const auto& item :formats ) {
        if(av_tio_format_map.find((AVPixelFormat)item.format) != av_tio_format_map.end()){
            return index;
        }
        index++;
    }
    return -1;
}

int32_t VideoFrameConvert::receive(IPin *input_pin, FramePointer frame)
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

int32_t VideoFrameConvert::requare(int32_t duration, const std::vector<PinIndex> &output_pins)
{
    (void)output_pins;
    return duration;
}

int32_t VideoFrameConvert::connect_chose_output_format(IPin *output_pin, int32_t index)
{
    (void)output_pin;
    (void)index;
    return 0;
}


int32_t VideoFrameConvert::property_changed(const std::string &property, Value &symbol)
{
    refresh_property();
    return 0;
}

void VideoFrameConvert::refresh_property()
{
    format_out_.width = properties_["width"];
    format_out_.height = properties_["height"];
    std::string fillmode = properties_["fillMode"];
    if(fillmode == "fill")
        output_fillmode_  = kStretchFill;
    else if(fillmode == "fit")
        output_fillmode_  = kAspectFit;
    else if(fillmode == "crop")
        output_fillmode_  = kAspectCrop;

    format_out_.format = AV_PIX_FMT_NONE;

    std::string format = properties_["format"];
    for(int index = kSoftwareFormatFirst; index < kSoftwareFormatCount; index++){
        if(format == g_software_format_info[index].name){
            for(auto& item : av_tio_format_map){
                if(item.second == index){
                    format_out_.format = (AVPixelFormat)item.first;
                }
            }
        }
    }
    update_pin_format(kOutputPin,0,0,format_out_);
}


};


