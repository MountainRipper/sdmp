#include "data_grabber.h"

namespace mr::sdmp {

COM_REGISTER_OBJECT(DataGrabber)

DataGrabber::DataGrabber()
{

}


int32_t sdmp::DataGrabber::initialize(IGraph *graph, const Value &config_value)
{
    create_general_pin(AVMEDIA_TYPE_UNKNOWN,kInputPin);
    create_general_pin(AVMEDIA_TYPE_UNKNOWN,kOutputPin);

    auto ret = GeneralFilter::initialize(graph,config_value);
    //get pre-set hanlder
    handler_ = (IFilterHandlerDataGrabber*)properties_["grabber"].as_pointer();
    return ret;
}


int32_t DataGrabber::connect_match_input_format(IPin *sender_pin,IPin *input_pin)
{
    (void)input_pin;
    const auto& formats = sender_pin->formats();
    int index = 0;
    for (const auto& item :formats ) {
        if(item.type >=0 && item.codec >= 0 && item.format >= 0){
            update_pin_format(kInputPin,0,0,item);
            update_pin_format(kOutputPin,0,0,item);
            if(handler_){
                handler_->grabber_get_format(id_,&item);
            }
            return index;
        }
        index++;
    }
    return -1;
}

int32_t DataGrabber::receive(IPin *input_pin, FramePointer frame)
{
    (void)input_pin;
    get_pin(kOutputPin,0)->deliver(frame);
    if(handler_){
        handler_->grabber_get_frame(id_,frame);
    }
    return 0;
}

int32_t DataGrabber::requare(int32_t duration, const std::vector<PinIndex> &output_pins)
{
    (void)output_pins;
    return duration;
}

int32_t DataGrabber::connect_chose_output_format(IPin *output_pin, int32_t index)
{
    (void)output_pin;
    (void)index;
    return 0;
}


int32_t DataGrabber::property_changed(const std::string &property, Value &symbol)
{
    if(property == "grabber"){
        handler_ = (IFilterHandlerDataGrabber*)symbol.as_pointer();
    }
    return 0;
}


};


