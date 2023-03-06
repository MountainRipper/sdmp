#include <libyuv.h>
#include "video_frame_convert.h"
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

    return ret;
}


int32_t VideoFrameConvert::connect_match_input_format(IPin *sender_pin,IPin *input_pin)
{
    (void)input_pin;
    const auto& formats = sender_pin->formats();
    int index = 0;
    for (const auto& item :formats ) {
        if(item.type >=0 && item.codec >= 0 && item.format >= 0){

        }
        index++;
    }
    return -1;
}

int32_t VideoFrameConvert::receive(IPin *input_pin, FramePointer frame)
{
    (void)input_pin;
    get_pin(kOutputPin,0)->deliver(frame);
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

    return 0;
}


};


