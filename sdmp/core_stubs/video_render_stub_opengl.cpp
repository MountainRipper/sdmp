#include "video_render_stub_opengl.h"
#include "core_includes.h"
#include "sdp_factory_implement.h"

//int32_t render_format_from_avformat(int32_t avformat){
//    if(avformat == AV_PIX_FMT_RGBA)             return kPixelFormatRGBA;
//    else if(avformat == AV_PIX_FMT_BGRA)        return kPixelFormatBGRA;
//    else if(avformat == AV_PIX_FMT_RGB24)       return kPixelFormatRGB;
//    else if(avformat == AV_PIX_FMT_BGR24)       return kPixelFormatBGR;
//    else if(avformat == AV_PIX_FMT_GRAY8)       return kPixelFormatLuminance;
//    else if(avformat == AV_PIX_FMT_GRAY8A)      return kPixelFormatLuminanceAlpha;
//    else if(avformat == AV_PIX_FMT_YUV420P)     return kPixelFormatYUV420P;
//    else if(avformat == AV_PIX_FMT_YUVJ420P)    return kPixelFormatYUV420P;
//    else if(avformat == AV_PIX_FMT_NV12)        return kPixelFormatNV12;
//    else if(avformat == AV_PIX_FMT_NV21)        return kPixelFormatNV21;
//    else if(avformat == AV_PIX_FMT_DRM_PRIME)   return kPixelFormatNV12;
//    else if(avformat == AV_PIX_FMT_VAAPI)       return kPixelFormatNV12;
//    return kPixelFormatNONE;
//}

#if defined (__USE_BAD_GPUPROCESSOR__)

#include "gpu_processor_interface.h"
#include "core_includes.h"
#include "sdp_factory_implement.h"

#define TEST_PIXEL_FORMAT 0

using namespace gpu_processor;

namespace sdp {


VideoRenderStubOpenGL::VideoRenderStubOpenGL()
{

}

VideoRenderStubOpenGL::~VideoRenderStubOpenGL()
{
    gpu_processor::VideoProcessorInterface::destroy_video_processor(processor_);
}

int32_t VideoRenderStubOpenGL::create(int width, int height, int32_t pixel_avformat,int32_t rotate)
{
    if(processor_ == nullptr){
        std::string filter_dir = Factory::factory()->script_dir();
        VideoProcessorInterface::init((filter_dir + "filters.lua").c_str(),filter_dir.c_str());
        processor_ = gpu_processor::VideoProcessorInterface::create_video_processor();

        DisplayType display_type = kDisplayTypeAuto;
        auto& features = Factory::factory()->features();
        auto it = features.find("graphicDriver");
        if(it != features.end()){
            std::string driver = it->second;
            if (driver == "egl") display_type = kDisplayTypeEGL;
            if (driver == "glx") display_type = kDisplayTypeGLX;
            if (driver == "wgl") display_type = kDisplayTypeWGL;
            if (driver == "agl") display_type = kDisplayTypeAGL;
            if (driver == "eagl") display_type = kDisplayTypeEAGL;
            if (driver == "wayland") display_type = kDisplayTypeWayland;
            if (driver == "linuxfb") display_type = kDisplayTypeEGL;
            if (driver == "directfb") display_type = kDisplayTypeEGL;
            //kDisplayTypeX11 kDisplayTypeDRM almost glx/egl
        }
        processor_->init(nullptr,display_type);
    }
    std::vector<std::string> filters;
    //filters.push_back("SepiaFilter");
    BufferPixelFormat render_format = (BufferPixelFormat)render_format_from_avformat((AVPixelFormat)pixel_avformat);
    processor_->create(filters,width,height,1.0,rotate,render_format);
    return 0;
}

int32_t VideoRenderStubOpenGL::set_viewport(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if(processor_)
        processor_->set_render_viewport(x,y,width,height);
    return 0;
}

int32_t VideoRenderStubOpenGL::use_external_fbo(int32_t fbo, int32_t texture, int32_t width, int32_t height)
{
    if(fbo && texture && width && height){
        processor_->use_external_fbo(fbo,texture,width,height);
    }
    return 0;
}

int32_t VideoRenderStubOpenGL::render(std::shared_ptr<sdp::Frame> frame)
{
    std::lock_guard<std::mutex> lock(frame_mutex_);
    if(cache_frames_.size() > max_cache_frames_){
        cache_frames_.pop();
    }
    cache_frames_.push(frame);
    //MP_LOG_DEAULT("render cache frames:{}",cache_frames_.size());
    return 0;
}

int32_t VideoRenderStubOpenGL::present()
{
    std::shared_ptr<sdp::Frame> fresh_frame;
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        if(cache_frames_.size()){
            fresh_frame = current_frame_ = cache_frames_.front();
            cache_frames_.pop();
        }
    }

    if(fresh_frame){
        int ret = 0;
#if (TEST_PIXEL_FORMAT != 0)
        auto sdp_frame = make_test_frame_from_yuv420p(render_frame);
        ret = processor_->process_avframe_video(sdp_frame->frame);
#else
        ret = processor_->process_avframe_video(fresh_frame->frame);
        if(ret == kErrorFallbackToSoftwareFrame){
            auto sw_frame = fresh_frame->transfer_software_frame();
            if(sw_frame){
                processor_->process_avframe_video(sw_frame->frame);
            }
        }
#endif
        if(current_frame_)
            processor_->render();
    }


    return 0;
}


const std::string &VideoRenderStubOpenGL::module_name()
{
    static const std::string id = "VideoRenderStubOpenGL";
    return id;
}

int32_t VideoRenderStubOpenGL::destory()
{
    delete this;
    return 0;
}


}
#endif
