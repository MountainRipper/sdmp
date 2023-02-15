#include "video_output_proxy.h"

namespace mr::sdmp {

COM_REGISTER_OBJECT(VideoOutputProxyFilter)

bool support_avformat(int32_t avformat){
    if(avformat == AV_PIX_FMT_RGBA)             return true;
    else if(avformat == AV_PIX_FMT_BGRA)        return true;
    else if(avformat == AV_PIX_FMT_RGB24)       return true;
    else if(avformat == AV_PIX_FMT_BGR24)       return true;
    else if(avformat == AV_PIX_FMT_GRAY8)       return true;
    else if(avformat == AV_PIX_FMT_GRAY8A)      return true;
    else if(avformat == AV_PIX_FMT_YUV420P)     return true;
    else if(avformat == AV_PIX_FMT_YUVJ420P)    return true;
    else if(avformat == AV_PIX_FMT_YUV444P)     return true;
    else if(avformat == AV_PIX_FMT_NV12)        return true;
    else if(avformat == AV_PIX_FMT_NV21)        return true;
    else if(avformat == AV_PIX_FMT_DRM_PRIME)   return true;
    else if(avformat == AV_PIX_FMT_VAAPI)       return true;
    return false;
}

VideoOutputProxyFilter::VideoOutputProxyFilter()
{

}

int32_t VideoOutputProxyFilter::initialize(IGraph *graph, const sol::table &config)
{
    create_general_pin(AVMEDIA_TYPE_AUDIO,kInputPin);
    GeneralFilter::initialize(graph,config);
    return 0;
}

int32_t VideoOutputProxyFilter::process_command(const std::string &command, const Value& param)
{
    std::lock_guard<std::mutex> lock(cache_mutex_);
    if(command == kGraphCommandSeek || command == kGraphCommandStop){
        while (frames_cache_.size()) {
            frames_cache_.pop();
        }
        MP_LOG_DEAULT("VideoOutputProxyFilter clear frames");
    }
    return GeneralFilter::process_command(command,param);
}

int32_t VideoOutputProxyFilter::connect_match_input_format(IPin *sender_pin,IPin *input_pin)
{
    const auto& formats = sender_pin->formats();
    int index = 0;
    for(auto& item : formats){
        if( support_avformat((AVPixelFormat)item.format) ){
            update_pin_format(kInputPin,0,0,item);

            if(item.fps > 0)
                frame_interval_ = 1000.0 / item.fps;
            else
                frame_interval_ = 50;//default to 20fps
            return index;
        }
        index++;
    }
    update_pin_format(kInputPin,0,0,sdmp::Format());    
    return -1;
}

int32_t VideoOutputProxyFilter::connect_chose_output_format(IPin *output_pin, int32_t index)
{
    (void)output_pin;
    (void)index;
    return 0;
}

int32_t VideoOutputProxyFilter::receive(IPin* input_pin,FramePointer frame)
{
    std::lock_guard<std::mutex> lock(cache_mutex_);

    if(status_ != kStatusRunning)
        return 0;
    if(frame == nullptr)
        return kErrorInvalidParameter;

    if(frame->frame == nullptr){
        if(frame->flag & kFrameFlagEos){
            switch_status(kStatusEos);
            return kSuccessed;
        }
        return kErrorInvalidParameter;
    }

    frames_cache_.push(frame);
    //MP_LOG_DEAULT("VideoOutputProxyFilter receive frame:{} count:{}",frame->frame->pts,frames_cache_.size());
    return 0;
}



int32_t VideoOutputProxyFilter::requare(int32_t duration,const std::vector<PinIndex>& output_pins)
{
    if(status_ == kStatusEos)
        return kErrorFilterEos;

    if(status_ != kStatusRunning)
        return 0;

    if(properties_["modePullPush"].as_bool() == false){
        //push automatically
        pull_render_sync();
    }

    std::lock_guard<std::mutex> lock(cache_mutex_);
    int32_t count = frames_cache_.size();

    int32_t hunger = properties_["cacheHungerFrames"].as_double();
    int32_t cache =  properties_["cacheFrames"].as_double();
    if( count <= hunger)
        return (cache-count) * frame_interval_;

    return 0;
}

int32_t VideoOutputProxyFilter::property_changed(const std::string& name,Value& symbol)
{
    return 0;
}

int32_t VideoOutputProxyFilter::append_observer(sdmp::IFilterExtentionVideoOutputProxy::Observer *observer)
{
    if(observer)
        observers_.insert(observer);
    return 0;
}

int32_t VideoOutputProxyFilter::remove_observer(sdmp::IFilterExtentionVideoOutputProxy::Observer *observer)
{
    if(observer)
        observers_.erase(observer);
    return 0;
}

int32_t VideoOutputProxyFilter::pull_render_sync()
{
    if(status_ != kStatusRunning)
        return 0;

    Value graph_pts;
    graph_->get_property(kGraphPropertyCurrentPts,graph_pts);

    std::lock_guard<std::mutex> lock(cache_mutex_);
    if(frames_cache_.empty())
        return 0;

    auto &frame = frames_cache_.front();
    int64_t global_pts = (int64_t)graph_pts.as_int64();
    int64_t cur_pts = frame->frame->pts;

    bool without_sync = properties_["withoutSync"].as_bool();

    //MP_LOG_DEAULT("video render check: frame:{} cur:{} {}",frame->frame->pts,global_pts,without_sync?"!SYNC":"");

    bool less_in_range = ((global_pts - cur_pts) <= properties_["lessRangeMs"].as_int64());
    if(!without_sync && (cur_pts >= global_pts || less_in_range))
        return 0;

    for(auto& render : observers_)
        render->proxy_render_frame(frame);

    frames_cache_.pop();
    return 0;
}


}

