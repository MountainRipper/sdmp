#ifndef DATAGRABBER_H
#define DATAGRABBER_H
#include "sdmp_general_filter.h"

namespace mr::sdmp {

COM_MULTITHREADED_OBJECT(
"4df88f74-7f56-41c4-8e18-fec71c8c7de9",
R"({
  "clsid": "4df88f74-7f56-41c4-8e18-fec71c8c7de9",
  "describe": "packing video frame to single image,and a sub-area manifest",
  "filtertype": ["VideoProcessor"],
  "name": "videoThumbnailPack",
  "properties": [
    {
      "name": "powOfTwo",
      "type": "bool",
      "value": false
    },
    {
      "name": "maxWidth",
      "type": "number",
      "value": 4096
    },
    {
      "name": "maxHeight",
      "type": "number",
      "value": 4096
    },
    {
      "name": "frameSize",
      "type": "number",
      "value": 64,
      "describe":"the size of the wider side,default 4096*4096#64=4096 frames. 4096*4096#128=1024 frame"
    },
    {
      "name": "fillMode",
      "type": "string",
      "value": "crop",
      "describe":"[fill,fit,crop,keep], [keep] use the source aspect ratio, others use frameSize*frameSize"
    }
  ],
  "type": "sdp-filter"
})",
VideoThumbnailPack)
, public GeneralFilterTypedAs<VideoThumbnailPack>
{
public:
    VideoThumbnailPack();
    COM_MAP_BEGINE(VideoThumbnailPack)
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
};

};
#endif // DATAGRABBER_H
