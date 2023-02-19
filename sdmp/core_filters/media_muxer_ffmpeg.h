#ifndef MEDIAMUXERFFMPEG_H
#define MEDIAMUXERFFMPEG_H

#include "sdp_general_filter.h"
namespace mr::sdmp {

COM_MULTITHREADED_OBJECT(
"4a58d1e0-a20f-11ed-a6b8-1b1bd03f2ac3",
R"({
  "clsid": "4a58d1e0-a20f-11ed-a6b8-1b1bd03f2ac3",
  "describe": "media muxer powered by ffmpeg",
  "filtertype": ["VideoOutput","AudioOutput"],
  "name": "mediaMuxerFFmpeg",
  "properties": [
    {
      "name": "duration",
      "type": "number",
      "value": 0.0
    },
    {
      "name": "formatName",
      "type": "string",
      "value": ""
    },
    {
      "name": "protocol",
      "type": "string",
      "value": ""
    },
    {
      "name": "uri",
      "type": "string",
      "value": ""
    }
  ],
  "type": "sdp-filter"
})",
MediaMuxerFFmpegFilter)
, public GeneralFilterTypedAs<MediaMuxerFFmpegFilter>
{
public:
    MediaMuxerFFmpegFilter();
    COM_MAP_BEGINE(MediaMuxerFFmpegFilter)
        COM_INTERFACE_ENTRY(IFilter)
    COM_MAP_END()
    // FilterBase interface
public:
    virtual int32_t initialize(IGraph *graph, const Value &filter_values) override;
    virtual int32_t connect_match_input_format(IPin *sender_pin,IPin *input_pin);
    virtual int32_t connect_chose_output_format(IPin *output_pin, int32_t index);
    virtual int32_t receive(IPin *input_pin, FramePointer frame);
    virtual int32_t requare(int32_t duration, const std::vector<PinIndex> &output_pins);
    virtual int32_t process_command(const std::string &command, const Value &param);
private:
    int32_t open_muxer();
    int32_t add_stream(const sdmp::Format& format);
    int32_t write_header();

    int32_t write_proc();
    int32_t start_write_thread();
    int32_t finish_write_thread();
private:
    AVFormatContext*                muxer_ = nullptr;
    bool                            header_writed_ = false;
    std::map<int32_t,int32_t>       pin_stream_map_;
    std::map<int32_t,sdmp::Rational> pin_timebase_map_;
    std::mutex                      write_mutex_;

    std::thread                     write_thread_;
    Queue<FramePointer>             packet_cache_;
    int32_t                         error_     = 0;
    bool                            quit_flag_ = false;

};

}

#endif // MEDIAMUXERFFMPEG_H
