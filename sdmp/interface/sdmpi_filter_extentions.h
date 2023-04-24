#ifndef SDPI_FILTER_EXTENTIONS_H
#define SDPI_FILTER_EXTENTIONS_H

#include <inttypes.h>
#include <string>
#include <memory>
#include <vector>
#include "sdmpi_objects.h"

extern "C"{
struct AVFormatContext;
}
namespace mr::sdmp {

///////////// extern pin interfaces
//filter extention interface for implement some standard function
COM_INTERFACE("40056668-a7e8-11eb-86d3-efcdd4820993",IAudioOutputParticipantPin)
    //return volume 0-100
    COM_METHOD( requare_samples(uint8_t* pcm,int32_t samples) )
};

COM_INTERFACE("00dbdc6c-a7eb-11eb-b2e1-ffc673986017",IFilterExtentionVideoOutputProxy)
    class Observer{
    public:
        virtual int32_t proxy_render_frame(std::shared_ptr<sdmp::Frame> frame) = 0;
    };

    COM_METHOD( append_observer(Observer* observer) )
    COM_METHOD( remove_observer(Observer* observer) )
    COM_METHOD( pull_render_sync() )
};

COM_INTERFACE("dbb6987a-e274-11ed-a101-d327fd9131b3",IFilterExtentionVideoRenderer)
    struct RenderParam{
        int32_t view_width = 0;
        int32_t view_height = 0;
        float rotate = 0;
        float scale_x = 1;
        float scale_y = 1;
        float offset_x = 0;
        float offset_y = 0;
    };
    COM_METHOD( render_video_frame(FramePointer frame,IFilterExtentionVideoRenderer::RenderParam param) )
    COM_METHOD( render_current_frame(IFilterExtentionVideoRenderer::RenderParam param))
    //not auto relese in destructor, need release manually in graphic thread
    COM_METHOD( release())
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
    COM_METHOD( get_source_media_info(Info& info) )
    COM_METHOD( active_cache_mode(bool active) )
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
    COM_METHOD_RET( int64_t, set_timeline(int64_t time) )
    COM_METHOD_RET( int64_t, get_timeline() )

    COM_METHOD_RET( TimelineStatus, get_timeline_status() )
    COM_METHOD( set_timeline_status(TimelineStatus status) )
};


//handlers for user implement to handle events
COM_INTERFACE("6ff5d082-a7ee-11eb-9eff-63cdc9c9d9c1",IFilterHandleMediaSourceCustomIO)
    COM_METHOD( use(AVFormatContext* context,const std::string& uri) )
    COM_METHOD( unuse() )
};


COM_INTERFACE("77a31830-a7ee-11eb-addf-fba0f3c250e5",IFilterHandlerDataGrabber)
    COM_METHOD( grabber_get_format(const std::string& id,const Format* format) )
    COM_METHOD( grabber_get_frame(const std::string& id,std::shared_ptr<sdmp::Frame> frame) )
};


#define CLSID_ATTENDANT_VIDEO_GRAPHIC_RENDERER "cb428b4a-e277-11ed-99d6-7b89cf2f8355"

}



#endif // SDPI_FILTER_EXTENTIONS_H
