#include "video_thumbnail_pack.h"

namespace mr::sdmp {

COM_REGISTER_OBJECT(VideoThumbnailPack)

VideoThumbnailPack::VideoThumbnailPack()
{

}


int32_t sdmp::VideoThumbnailPack::initialize(IGraph *graph, const Value &filter_values)
{
    create_general_pin(AVMEDIA_TYPE_UNKNOWN,kInputPin);
    create_general_pin(AVMEDIA_TYPE_UNKNOWN,kOutputPin);

    auto ret = GeneralFilter::initialize(graph,filter_values);
    //get pre-set hanlder
    handler_ = (IFilterHandlerDataGrabber*)properties_["grabber"].as_pointer();
    return ret;
}


int32_t VideoThumbnailPack::connect_match_input_format(IPin *sender_pin,IPin *input_pin)
{
    (void)input_pin;
    const auto& formats = sender_pin->formats();
    int index = 0;
    for (const auto& item :formats ) {
        if(av_tio_format_map.find((AVPixelFormat)item.format) != av_tio_format_map.end()){
            sync_update_pin_format(kOutputPin,0,0,item);
            return index;
        }
        index++;
    }
    return -1;
}

int32_t VideoThumbnailPack::receive(IPin *input_pin, FramePointer frame)
{
    (void)input_pin;
    get_pin(kOutputPin,0)->deliver(frame);
    if(handler_){
        handler_->grabber_get_frame(id_,frame);
    }
    return 0;
}

int32_t VideoThumbnailPack::requare(int32_t duration, const std::vector<PinIndex> &output_pins)
{
    (void)output_pins;
    return duration;
}

int32_t VideoThumbnailPack::connect_chose_output_format(IPin *output_pin, int32_t index)
{
    (void)output_pin;
    (void)index;
    return 0;
}


int32_t VideoThumbnailPack::property_changed(const std::string &property, Value &symbol)
{
    if(property == "grabber"){
        handler_ = (IFilterHandlerDataGrabber*)symbol.as_pointer();
    }
    return 0;
}


};


