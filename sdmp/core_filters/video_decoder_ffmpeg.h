#ifndef VIDEODECODERFFMPEG_H
#define VIDEODECODERFFMPEG_H
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "sdp_general_filter.h"

namespace mr::sdmp {

COM_MULTITHREADED_OBJECT(
"67f6c00e-a20f-11ed-a870-e76493e9acc1",
R"({
  "clsid": "67f6c00e-a20f-11ed-a870-e76493e9acc1",
  "describe": "video decoder powered by ffmpeg",
  "filtertype": ["VideoDecoder"],
  "name": "videoDecoderFFmpeg",
  "properties": [
    {
      "name": "decoderName",
      "type": "string",
      "value": ""
    },
    {
      "name": "framesKeepCache",
      "type": "number",
      "value": 5.0
    },
    {
      "name": "framesPreDecode",
      "type": "number",
      "value": 5.0
    },
    {
      "name": "hardwareApi",
      "type": "string",
      "value": ""
    }
  ],
  "type": "sdp-filter"
})",
VideoDecoderFFmpegFilter)
, public GeneralFilterObjectRoot<VideoDecoderFFmpegFilter>
{
public:    
    VideoDecoderFFmpegFilter();
    ~VideoDecoderFFmpegFilter();

    COM_MAP_BEGINE(VideoDecoderFFmpegFilter)
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
public:
    AVPixelFormat hardware_format_requare();
private:
    int32_t open_decoder(const Format& format);
    int32_t try_open_hardware_context();
    int32_t close_decoder();
    int32_t decode_proc();
    int32_t decode_a_frame(FramePointer frame);
private:
    const AVCodec*  codec_          = nullptr;
    AVCodecContext* decoder_        = nullptr;
    std::string     hw_driver_;
    AVBufferRef*    hw_device_ctx_  = nullptr;
    AVPixelFormat   hw_format_      = AV_PIX_FMT_NONE;
    Format          format_out_;
    std::thread     decode_thread_;
    std::mutex      decode_mutex_;
    std::condition_variable decode_condition_;
    bool            quit_decode_    = false;
    StatusWait      pause_wait_;

    int64_t         current_decode_frames_  = 0;
    int64_t         has_decode_frames_      = 0;
    int64_t         decode_to_frames_       = 0;
    std::queue<FramePointer> received_frames_;
};
}

#endif // MEDIASOURCEFILTER_H
