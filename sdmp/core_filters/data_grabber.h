#ifndef DATAGRABBER_H
#define DATAGRABBER_H
#include "sdp_general_filter.h"

namespace sdp {

COM_MULTITHREADED_OBJECT(
"d331a236-a209-11ed-85db-27bb4fb6ae05",
R"({
  "clsid": "d331a236-a209-11ed-85db-27bb4fb6ae05",
  "describe": "data grabber for grab stream data to memory",
  "filtertype": ["AudioProcessor","VideoProcessor"],
  "name": "dataGrabber",
  "properties": [
    {
      "name": "grabber",
      "type": "pointer",
      "value": 0
    }
  ],
  "type": "sdp-filter"
})",
DataGrabber)
, public GeneralFilterObjectRoot<DataGrabber>
{
public:
    DataGrabber();
    COM_MAP_BEGINE(DataGrabber)
        COM_INTERFACE_ENTRY(IFilter)
    COM_MAP_END()
    // FilterBase interface
public:
    virtual int32_t initialize(IGraph *graph, const table &config) override;
    virtual int32_t connect_match_input_format(IPin *sender_pin,IPin *input_pin) override;
    virtual int32_t connect_chose_output_format(IPin *output_pin, int32_t index) override;
    virtual int32_t receive(IPin *input_pin, FramePointer frame) override;
    virtual int32_t requare(int32_t duration, const std::vector<PinIndex> &output_pins) override;
    // GeneralFilter interface
public:
    virtual int32_t property_changed(const std::string &property, NativeValue &symbol) override;
private:
    IFilterHandlerDataGrabber* handler_ = nullptr;
};

};
#endif // DATAGRABBER_H
