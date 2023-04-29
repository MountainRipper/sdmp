#include "null_output.h"

namespace mr::sdmp {

COM_REGISTER_OBJECT(NullOutput)

NullOutput::NullOutput()
{

}


int32_t sdmp::NullOutput::initialize(IGraph *graph, const Value &filter_values)
{
    create_general_pin(AVMEDIA_TYPE_UNKNOWN,kInputPin);
    auto ret = GeneralFilter::initialize(graph,filter_values);
    return ret;
}


int32_t NullOutput::connect_match_input_format(IPin *sender_pin,IPin *input_pin)
{
    (void)input_pin;
    const auto& formats = sender_pin->formats();
    int32_t select_format = properties_["selectFormat"];
    if(select_format >= formats.size())
        select_format = 0;
    if(formats.size()){
        sync_update_pin_format(kInputPin,0,0,formats[select_format]);
        return 0;
    }
    return -1;
}

int32_t NullOutput::receive(IPin *input_pin, FramePointer frame)
{
    return 0;
}

int32_t NullOutput::requare(int32_t duration, const std::vector<PinIndex> &output_pins)
{
    (void)output_pins;
    return duration;
}

int32_t NullOutput::connect_chose_output_format(IPin *output_pin, int32_t index)
{
    (void)output_pin;
    (void)index;
    return 0;
}


int32_t NullOutput::property_changed(const std::string &property, Value &symbol)
{
    return 0;
}


};


