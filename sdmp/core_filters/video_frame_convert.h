#ifndef DATAGRABBER_H
#define DATAGRABBER_H
#include "sdmp_general_filter.h"

namespace mr::sdmp {

class VideoFrameConvertPrivate;

COM_MULTITHREADED_OBJECT(
"7569e094-b9a3-11ed-a62d-03f03fb39753",
R"({
  "clsid": "7569e094-b9a3-11ed-a62d-03f03fb39753",
  "describe": "video software frame format convert",
  "filtertype": ["VideoProcessor"],
  "name": "videoFrameConvert",
  "properties": [
    {
      "name": "format",
      "type": "string",
      "value": "I420",
      "describe":"I420,YV12(false),NV12,NV21,I422,NV16,NV61(false),YUYV422,YVYU422,UYVY422,I444,NV24,NV42,YUV444(false),RGB24,BGR24,RGBA32,BGRA32,ARGB32,ABGR32,GRAY8,GRAY8A"
    },
    {
      "name": "width",
      "type": "number",
      "value": 0
    },
    {
      "name": "height",
      "type": "number",
      "value": 0
    },
    {
      "name": "fillMode",
      "type": "string",
      "value": "fit",
      "describe":"fill,fit,crop"
    }
  ],
  "type": "sdp-filter"
})",
VideoFrameConvert)
, public GeneralFilterTypedAs<VideoFrameConvert>
{
public:
    VideoFrameConvert();
    COM_MAP_BEGINE(VideoFrameConvert)
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
    void refresh_property();
private:
    int32_t output_fillmode_ = kStretchFill;
    Format  format_out_;
};

};
#endif // DATAGRABBER_H
