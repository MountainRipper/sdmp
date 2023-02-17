#ifndef MEDIASOURCEFFMPEG_H
#define MEDIASOURCEFFMPEG_H
#include <thread>
#include <mutex>
#include <condition_variable>
#include <logger.h>
#include "sdp_general_filter.h"
namespace mr::sdmp {

COM_MULTITHREADED_OBJECT(
"42feea38-a20f-11ed-85d2-cb2c531c5d11",
R"({
  "clsid": "42feea38-a20f-11ed-85d2-cb2c531c5d11",
  "describe": "media source powered by ffmpeg",
  "filtertype": ["AudioInput","VideoInput"],
  "name": "mediaSourceFFmpeg",
  "properties": [
    {
      "name": "accurateSeek",
      "type": "bool",
      "value": false
    },
    {
      "name": "customIO",
      "type": "string",
      "value": ""
    },
    {
      "name": "customIOCheckHandler",
      "type": "lua_function",
      "value": null
    },
    {
      "name": "duration",
      "type": "number",
      "value": 0.0
    },
    {
      "name": "exceptionHandler",
      "type": "lua_function",
      "value": null
    },
    {
      "name": "fps",
      "type": "number",
      "value": 0.0
    },
    {
      "name": "preBufferMs",
      "type": "number",
      "value": 1000.0
    },
    {
      "name": "protocol",
      "type": "string",
      "value": ""
    },
    {
      "name": "readingMode",
      "type": "number",
      "value": 0.0
    },
    {
      "name": "seekBackMs",
      "type": "number",
      "value": 200.0
    },
    {
      "name": "seekBackReConnect",
      "type": "bool",
      "value": true
    },
    {
      "name": "timeout",
      "type": "number",
      "value": 30000.0
    },
    {
      "name": "uri",
      "type": "string",
      "value": ""
    }
  ],
  "type": "sdp-filter"
})",
MediaSourceFFmpegFilter)
        , public GeneralFilterTypedAs<MediaSourceFFmpegFilter>
        , public IFilterExtentionMediaCacheSource
{
public:
    MediaSourceFFmpegFilter();
    ~MediaSourceFFmpegFilter();

    COM_MAP_BEGINE(MediaSourceFFmpegFilter)
        COM_INTERFACE_ENTRY(IFilter)
        COM_INTERFACE_ENTRY(IFilterExtentionMediaCacheSource)
    COM_MAP_END()
    // FilterBase interface
public:
    virtual int32_t initialize(IGraph *graph, const Value &config_value);
    virtual int32_t process_command(const std::string &command, const Value& param);
    virtual int32_t connect_match_input_format(IPin *sender_pin,IPin *input_pin);
    virtual int32_t connect_chose_output_format(IPin *output_pin, int32_t index);
    virtual int32_t receive(IPin* input_pin,FramePointer frame);
    virtual int32_t requare(int32_t duration,const std::vector<PinIndex>& output_pins);

    // FilterGeneral interface
public:
    virtual int32_t property_changed(const std::string& name, Value &symbol);

    // IFilterExtentionMediaCacheSource interface
public:
    virtual int32_t get_source_media_info(IFilterExtentionMediaCacheSource::Info& info) override;
    virtual int32_t active_cache_mode(bool active) override;
private:
    int32_t open_media(const std::string& uri,bool reconnect_in_proc = false);
    int32_t close();
    int32_t close_context();
    int32_t do_seek(int32_t ms, int32_t flag = AVSEEK_FLAG_BACKWARD);
    int32_t reading_proc();
    int32_t reset_timeout();
    int32_t check_timeout();
    int32_t reset_position();
    int32_t throw_job_error(const std::string& reson);
private:
    std::string         uri_;
    AVFormatContext     *media_file_ = nullptr;
    StatusWait          pause_wait_;
    std::thread         reading_thread_;
    std::condition_variable read_condition_;
    std::mutex          read_mutex_;
    std::map<int32_t,PinPointer> stream_pins_map_;
    int64_t             request_read_to_        = 0;
    bool                quit_reading_flag_      = false;
    int64_t             current_read_pts_       = -1;
    int64_t             skip_read_pts_to_       = -1;
    bool                seeked_                 = false;

    uint64_t            timeout_check_start_    = 0;
    int64_t             timeout_check_ms_       = 0;
    bool                network_timeout_        = false;
    int64_t             reconnect_count_        = 10;
    bool                media_loaded_            = false;

    bool                destructor_             = false;
    bool                caching_mode_           = false;

    int32_t             keyframes_read_         = 0;
    ComPointer<IFilterHandleMediaSourceCustomIO> custom_io_;
    MP_TIMER_DEFINE(timeout_timer_);
};
}

#endif // MEDIASOURCEFILTER_H
