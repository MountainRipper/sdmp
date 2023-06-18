#include "video_graphic_renderer.h"

namespace mr::sdmp{


COM_REGISTER_OBJECT(VideoGraphicRenderer)

VideoGraphicRenderer::VideoGraphicRenderer()
{

}

int32_t VideoGraphicRenderer::render_video_frame(FramePointer frame, IFilterExtentionVideoRenderer::RenderParam param)
{
    if(frame->frame == nullptr)
        return kErrorInvalidFrame;
    if(frame->frame->width <= 0 || frame->frame->height <= 0 || frame->frame->format < 0)
        return kErrorInvalidFrame;

    auto tio_frame = frame_to_tio(*frame->frame);
    if(tio_frame.format == mr::tio::kSoftwareFormatNone)
        return kErrorFormatNotSupport;

    if(shader_ == nullptr || shader_->format() != tio_frame.format)
        shader_ = TextureIO::create_reference_shader(mr::tio::kGraphicApiOpenGL,tio_frame.format);

    if(textures.context[0] == 0){
        SoftwareFrameWithMemory temp_frame(kSoftwareFormatI420,128,128);
        temp_frame.alloc();
        //make 3 textures from i420, so all format can use
        textures.api = kGraphicApiOpenGL;
        TextureIO::create_texture(temp_frame,textures);
    }

    auto ret = TextureIO::software_frame_to_graphic(tio_frame,textures);
    if(ret < 0)
        return ret;

    shader_->use();
    return shader_->render(textures,
                    {param.view_x ,
                     param.view_y ,
                     param.view_width ,
                     param.view_height,
                     param.rotate     ,
                     param.scale_x    ,
                     param.scale_y    ,
                     param.offset_x   ,
                     param.offset_y   });

}

int32_t VideoGraphicRenderer::render_current_frame(IFilterExtentionVideoRenderer::RenderParam param)
{
    if(!shader_)
        return kErrorResourceNotFound;

    shader_->use();
    return shader_->render(textures,
                           {param.view_x ,
                            param.view_y ,
                            param.view_width ,
                            param.view_height,
                            param.rotate     ,
                            param.scale_x    ,
                            param.scale_y    ,
                            param.offset_x   ,
                            param.offset_y   });
}

int32_t VideoGraphicRenderer::release()
{
    shader_ = nullptr;
    return TextureIO::release_texture(textures);
}


}
