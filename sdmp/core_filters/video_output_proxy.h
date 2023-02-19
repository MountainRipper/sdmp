#ifndef MEDIADEMUXER_H
#define MEDIADEMUXER_H
#include <queue>
#include <mutex>
#include "sdp_general_filter.h"
#include "sdpi_filter_extentions.h"
namespace mr::sdmp {

COM_MULTITHREADED_OBJECT(
"890e816e-a20f-11ed-bf35-03643b1fe0a4",
R"({
  "clsid": "890e816e-a20f-11ed-bf35-03643b1fe0a4",
  "describe": "video output proxy, use external real renderer",
  "filtertype": ["VideoOutput"],
  "name": "videoOutputProxy",
  "properties": [
    {
      "name": "cacheFrames",
      "type": "number",
      "value": 8.0
    },
    {
      "name": "cacheHungerFrames",
      "type": "number",
      "value": 4.0
    },
    {
      "name": "lessRangeMs",
      "type": "number",
      "value": 3.0
    },
    {
      "name": "modePullPush",
      "type": "bool",
      "value": true
    },
    {
      "name": "withoutSync",
      "type": "bool",
      "value": false
    }
  ],
  "type": "sdp-filter"
})",
VideoOutputProxyFilter)
    , public GeneralFilterTypedAs<VideoOutputProxyFilter>
    , public IFilterExtentionVideoOutputProxy
{
public:
    VideoOutputProxyFilter();

    COM_MAP_BEGINE(VideoOutputProxyFilter)
        COM_INTERFACE_ENTRY(IFilter)
        COM_INTERFACE_ENTRY(IFilterExtentionVideoOutputProxy)
    COM_MAP_END()
    // FilterBase interface
public:
    virtual int32_t initialize(IGraph *graph, const Value &filter_values);
    virtual int32_t process_command(const std::string &command, const Value& param);
    virtual int32_t connect_match_input_format(IPin *sender_pin,IPin *input_pin);
    virtual int32_t connect_chose_output_format(IPin *output_pin, int32_t index);
    virtual int32_t receive(IPin* input_pin,FramePointer frame);
    virtual int32_t requare(int32_t duration,const std::vector<PinIndex>& output_pins);

    // FilterGeneral interface
public:
    virtual int32_t property_changed(const std::string& name,Value& symbol);
    // IFilterExtentionVideoOutputProxy interface
public:
    virtual int32_t append_observer(IFilterExtentionVideoOutputProxy::Observer *observer);
    virtual int32_t remove_observer(IFilterExtentionVideoOutputProxy::Observer *observer);
    virtual int32_t pull_render_sync();

private:
    std::mutex cache_mutex_;
    std::set<IFilterExtentionVideoOutputProxy::Observer*> observers_;
    std::queue<FramePointer> frames_cache_;
    double frame_interval_ = 50;
};

}

#endif // MEDIASOURCEFILTER_H
