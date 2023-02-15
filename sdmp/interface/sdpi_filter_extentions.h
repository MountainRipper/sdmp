#ifndef SDPI_FILTER_EXTENTIONS_H
#define SDPI_FILTER_EXTENTIONS_H

#include <inttypes.h>
#include <string>
#include <memory>
#include <vector>
#include "sdpi_objects.h"

extern "C"{
struct AVFormatContext;
}
namespace mr::sdmp {

///////////// extern pin interfaces
//filter extention interface for implement some standard function
COM_INTERFACE("40056668-a7e8-11eb-86d3-efcdd4820993",IAudioOutputParticipantPin)
    //return volume 0-100
    virtual int32_t requare_samples(uint8_t* pcm,int32_t samples) = 0;
};

COM_INTERFACE("00dbdc6c-a7eb-11eb-b2e1-ffc673986017",IFilterExtentionVideoOutputProxy)
    class Observer{
    public:
        virtual int32_t proxy_render_frame(std::shared_ptr<sdmp::Frame> frame) = 0;
    };

    virtual int32_t append_observer(Observer* observer) = 0;
    virtual int32_t remove_observer(Observer* observer) = 0;
    virtual int32_t pull_render_sync() = 0;
};


#define kMediaCacheCommandPlay  "media-cache-source-play"
#define kMediaCacheCommandSeek  "media-cache-source-seek"
#define kMediaCacheCommandPause "media-cache-source-pause"
#define kMediaCacheCommandStop  "media-cache-source-stop"
COM_INTERFACE("61c64244-a7ee-11eb-9ca2-dfd54d1aef9e",IFilterExtentionMediaCacheSource)
    struct Info{
        std::string uri_key;
        std::string protocol;
        std::string format_name;
        int32_t     duration = 0;
        std::vector<IPin*> output_pins;
    };
    virtual int32_t get_source_media_info(Info& info) = 0;
    virtual int32_t active_cache_mode(bool active) = 0;
};


COM_INTERFACE("67590bce-a7ee-11eb-817e-035a8fe167ce",IFilterExtentionTimeline)
    enum TimelineStatus{
        kTimelineNone,
        kTimelineRunning,
        kTimelineDisactive,
        kTimelineBlocking,
        kTimelineOver
    };
public:
    virtual int64_t set_timeline(int64_t time) = 0;
    virtual int64_t get_timeline() = 0;

    virtual TimelineStatus get_timeline_status() = 0;
    virtual int32_t set_timeline_status(TimelineStatus status) = 0;
};


//handlers for user implement to handle events
COM_INTERFACE("6ff5d082-a7ee-11eb-9eff-63cdc9c9d9c1",IFilterHandleMediaSourceCustomIO)
    virtual int32_t use(AVFormatContext* context,const std::string& uri) = 0;
    virtual int32_t unuse() = 0;
};


COM_INTERFACE("77a31830-a7ee-11eb-addf-fba0f3c250e5",IFilterHandlerDataGrabber)
    virtual int32_t grabber_get_format(const std::string& id,const Format* format) = 0;
    virtual int32_t grabber_get_frame(const std::string& id,std::shared_ptr<sdmp::Frame> frame) = 0;
};

}



#endif // SDPI_FILTER_EXTENTIONS_H
