#ifndef VIDEOGRAPHICRENDERER_H
#define VIDEOGRAPHICRENDERER_H
#include "core_includes.h"
#include "sdmpi_filter_extentions.h"
#include <tio/tio_hardware_graphic.h>
using namespace mr::sdmp;

namespace mr::sdmp{


COM_MULTITHREADED_OBJECT("cb428b4a-e277-11ed-99d6-7b89cf2f8355","",VideoGraphicRenderer)
, public mr::sdmp::IFilterExtentionVideoRenderer
{
public:
    COM_MAP_BEGINE(VideoGraphicRenderer)
        COM_INTERFACE_ENTRY(IFilterExtentionVideoRenderer)
    COM_MAP_END()
    VideoGraphicRenderer();

    virtual int32_t render_video_frame(FramePointer frame,IFilterExtentionVideoRenderer::RenderParam param) override;
    virtual int32_t render_current_frame(IFilterExtentionVideoRenderer::RenderParam param) override;
    virtual int32_t release();
private:
    std::shared_ptr<mr::tio::ReferenceShader> shader_;
    GraphicTexture textures;
};

}
#endif // VIDEOGRAPHICRENDERER_H
