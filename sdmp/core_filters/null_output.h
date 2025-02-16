#ifndef DATAGRABBER_H
#define DATAGRABBER_H
#include "sdmp_general_filter.h"

namespace mr::sdmp {

COM_MULTITHREADED_OBJECT(
"e8669380-ac23-445b-a6a6-6f3c59bfb7a3",
R"({
  "clsid": "e8669380-ac23-445b-a6a6-6f3c59bfb7a3",
  "describe": "a null output render,which do not want really rendering but need A/V data",
  "filtertype": ["AudioOutput","VideoOutput"],
  "name": "nullOutput",
  "properties": [
    {
      "name": "selectFormat",
      "type": "number",
      "value": 0
    }
  ],
  "type": "sdp-filter"
})",
NullOutput)
, public GeneralFilterTypedAs<NullOutput>
{
public:
    NullOutput();
    COM_MAP_BEGINE(NullOutput)
        COM_INTERFACE_ENTRY(IFilter)
    COM_MAP_END()
    // FilterBase interface
public:
    virtual int32_t initialize(IGraph *graph, const Value &filter_values) override;
    virtual int32_t connect_match_input_format(IPin *sender_pin,IPin *input_pin) override;
    virtual int32_t connect_chose_output_format(IPin *output_pin, int32_t index) override;
    virtual int32_t receive(IPin *input_pin, FramePointer frame) override;
    virtual int32_t requare(int32_t duration, const std::vector<PinIndex> &output_pins) override;
    // GeneralFilter interface
public:
    virtual int32_t property_changed(const std::string &property, Value &symbol) override;
private:
    IFilterHandlerDataGrabber* handler_ = nullptr;
    std::chrono::steady_clock::time_point start_point_;
    int64_t last_ms_linear_ = 0;
    int64_t current_recv_ms_ = 0;
};

};
#endif // DATAGRABBER_H
