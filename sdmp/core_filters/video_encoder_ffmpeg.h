#ifndef VIDEOENCODERFFMPEG_H
#define VIDEOENCODERFFMPEG_H

#include "sdp_general_filter.h"

namespace mr::sdmp {

COM_MULTITHREADED_OBJECT(
"81e64228-a20f-11ed-bf09-eb12fed8b6b8",
R"({
  "clsid": "81e64228-a20f-11ed-bf09-eb12fed8b6b8",
  "describe": "video encoder powered by ffmpeg",
  "filtertype": ["VideoEncoder"],
  "name": "videoEncoderFFmpeg",
  "properties": [
    {
      "name": "adviceUri",
      "type": "string",
      "value": ""
    },
    {
      "name": "bframeMax",
      "type": "number",
      "value": 3.0
    },
    {
      "name": "bitrate",
      "type": "number",
      "value": 4096000.0
    },
    {
      "name": "bitrateMaxScale",
      "type": "number",
      "value": 1.2
    },
    {
      "name": "bitrateMinScale",
      "type": "number",
      "value": 0.8
    },
    {
      "name": "encoderId",
      "type": "number",
      "value": 27.0
    },
    {
      "name": "encoderName",
      "type": "string",
      "value": "libx264"
    },
    {
      "name": "hardwareApi",
      "type": "string",
      "value": ""
    },
    {
      "name": "keyframeInterval",
      "type": "number",
      "value": 5.0
    },
    {
      "name": "preset",
      "type": "string",
      "value": "veryfast"
    },
    {
      "name": "qMax",
      "type": "number",
      "value": 51.0
    },
    {
      "name": "qMin",
      "type": "number",
      "value": 10.0
    },
    {
      "name": "tune",
      "type": "string",
      "value": "film"
    }
  ],
  "type": "sdp-filter"
})",
VideoEncoderFFmpegFilter)
    , public GeneralFilterObjectRoot<VideoEncoderFFmpegFilter>
{
public:
    VideoEncoderFFmpegFilter();
    ~VideoEncoderFFmpegFilter();

    COM_MAP_BEGINE(VideoEncoderFFmpegFilter)
        COM_INTERFACE_ENTRY(IFilter)
    COM_MAP_END()
    // FilterBase interface
public:
    virtual int32_t initialize(IGraph *graph, const sol::table &config);
    virtual int32_t connect_match_input_format(IPin *sender_pin,IPin *input_pin);
    virtual int32_t connect_chose_output_format(IPin *output_pin, int32_t index);
    virtual int32_t receive(IPin *input_pin, FramePointer frame);
    virtual int32_t requare(int32_t duration, const std::vector<PinIndex> &output_pins);
    virtual int32_t process_command(const std::string &command, const Value &param);

private:
    void    encode_proc();
    int32_t open_encoder(const Format &format);
    int32_t close_encoder();
    int32_t encode_a_frame(FramePointer frame);
private:
    AVCodecContext *codec_context = nullptr;
    AVCodecParameters* parameters_ = nullptr;
    Queue<FramePointer>  frames_;

    std::thread     encode_thread_;
    std::mutex      encode_mutex_;
    std::condition_variable encode_condition_;
    bool            quit_encode_    = false;
    StatusWait      pause_wait_;
};
}

#endif // VIDEOENCODERFFMPEG_H
