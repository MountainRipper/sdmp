#include "null_output.h"
namespace mr::sdmp {

COM_REGISTER_OBJECT(NullOutput)

NullOutput::NullOutput()
{
    start_point_ = std::chrono::steady_clock::time_point();
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
    if(frame->frame){
        current_recv_ms_ = frame->frame->pts;
    }
    else if(frame->packet){
        current_recv_ms_ = frame->packet->pts;
    }
    return 0;
}

int32_t NullOutput::requare(int32_t duration, const std::vector<PinIndex> &output_pins)
{
    auto now = std::chrono::steady_clock::now();
    if(start_point_.time_since_epoch().count() == 0){
        start_point_ = now;
        last_ms_linear_ = 0;
    }
    auto ms_linear = std::chrono::duration_cast<std::chrono::milliseconds>(now-start_point_).count();
    auto diff = ms_linear - last_ms_linear_;

    if(diff < 500)
        return 0;
    last_ms_linear_ = ms_linear;

    duration = diff;
    static int a = 0;
    if(a++ % 100 == 0)
        MR_INFO(">>>>>>> ms:{}",diff);

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


