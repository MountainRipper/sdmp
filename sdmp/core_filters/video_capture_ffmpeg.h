#ifndef VIDEODECODERAUTO_H
#define VIDEODECODERAUTO_H
#include "sdp_general_filter.h"

namespace mr::sdmp {

COM_MULTITHREADED_OBJECT(
"60f312bc-a20f-11ed-8732-c737d15a084c",
R"({
  "clsid": "60f312bc-a20f-11ed-8732-c737d15a084c",
  "describe": "video device capture, powered by ffmpeg libavdevice",
  "filtertype": ["VideoInput"],
  "name": "videoCapture",
  "properties": [],
  "type": "sdp-filter"
})",
VideoCaptureFFmpegFilter)
, public GeneralFilterObjectRoot<VideoCaptureFFmpegFilter>
{
public:    
    VideoCaptureFFmpegFilter();

    COM_MAP_BEGINE(VideoCaptureFFmpegFilter)
        COM_INTERFACE_ENTRY(IFilter)
    COM_MAP_END()
    // FilterBase interface
public:
    virtual int32_t initialize(IGraph *graph, const sol::table &config);
    virtual int32_t process_command(const std::string &command, const Value& param);
    virtual int32_t connect_match_input_format(IPin *sender_pin,IPin *input_pin);
    virtual int32_t connect_chose_output_format(IPin *output_pin, int32_t index);
    virtual int32_t receive(IPin* input_pin,FramePointer frame);
    virtual int32_t requare(int32_t duration,const std::vector<PinIndex>& output_pins);

    // FilterGeneral interface
public:
    virtual int32_t property_changed(const std::string& name,Value& symbol);

private:
    int32_t open_device();
};

}

#endif // MEDIASOURCEFILTER_H
