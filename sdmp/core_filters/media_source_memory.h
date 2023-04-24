#ifndef MEDIASOURCEMEMORY_H
#define MEDIASOURCEMEMORY_H

#include "sdmp_general_filter.h"

namespace mr::sdmp {

COM_MULTITHREADED_OBJECT(
"5b4c21c8-a20f-11ed-a8f0-0fe7bf0e7c84",
R"({
  "clsid": "5b4c21c8-a20f-11ed-a8f0-0fe7bf0e7c84",
  "describe": "media source from memory push",
  "filtertype": ["VideoInput","AudioInput"],
  "name": "mediaSourceMemory",
  "properties": [
    {
      "name": "duration",
      "type": "number",
      "value": 0.0
    },
    {
      "name": "pushGrabber",
      "type": "pointer",
      "value": 0
    }
  ],
  "type": "sdp-filter"
})",
MediaSourceMemoryFilter)
, public GeneralFilterTypedAs<MediaSourceMemoryFilter>
, public IFilterHandlerDataGrabber
{
public:
    MediaSourceMemoryFilter();

    COM_MAP_BEGINE(MediaSourceMemoryFilter)
        COM_INTERFACE_ENTRY(IFilter)
        COM_INTERFACE_ENTRY(IFilterHandlerDataGrabber)
    COM_MAP_END()
    // FilterBase interface
public:
    int32_t initialize(IGraph *graph, const Value &config);
    int32_t connect_match_input_format(IPin *sender_pin,IPin *input_pin);
    int32_t connect_chose_output_format(IPin *output_pin, int32_t index);
    int32_t receive(IPin *input_pin, FramePointer frame);
    int32_t requare(int32_t duration, const std::vector<PinIndex> &output_pins);
    int32_t process_command(const std::string& command,const Value& param);
    // IFilterHandlerDataGrabber interface
public:
    int32_t grabber_get_format(const std::string &id, const Format *format);
    int32_t grabber_get_frame(const std::string &id, FramePointer frame);
private:
    Format output_format_;
    Queue<FramePointer> frames_;
};


}

#endif // MEDIASOURCEMEMORY_H
