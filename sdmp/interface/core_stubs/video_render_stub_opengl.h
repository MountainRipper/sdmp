#ifndef VIDEORENDERSTUBOPENGL_H
#define VIDEORENDERSTUBOPENGL_H

#include <memory>
#include <mutex>
#include <queue>
#include "sdpi_basic_declears.h"

//int32_t render_format_from_avformat(int32_t avformat);

#if defined (__USE_BAD_GPUPROCESSOR__)

namespace gpu_processor {
class VideoProcessorInterface;
}
namespace sdp {
class Frame;
}

namespace sdp {

enum RenderType{
    kRenderTypeAuto,
    kRenderTypeDRM,
    kRenderTypeEGL,
    kRenderTypeGLX,
    kRenderTypeX11,
    kRenderTypeWayland,
    kRenderTypeWGL,
    kRenderTypeAGL,
    kRenderTypeEAGL,
};

class VideoRenderStubOpenGL : public ISdpStub
{
public:
    VideoRenderStubOpenGL();
    ~VideoRenderStubOpenGL();
    int32_t create(int width, int height, int32_t pixel_avformat, int32_t rotate);
    int32_t set_viewport(int32_t x,int32_t y,int32_t width,int32_t height);
    int32_t use_external_fbo(int32_t fbo, int32_t texture,int32_t width,int32_t height);
    int32_t render(std::shared_ptr<sdp::Frame> frame);
    int32_t present();
    // IStub interface
public:
    virtual const std::string &module_name();
    virtual int32_t destory();
private:
    std::shared_ptr<sdp::Frame> make_test_frame(FramePointer src_frame);
private:
    gpu_processor::VideoProcessorInterface* processor_ = nullptr;
    std::queue<std::shared_ptr<sdp::Frame>> cache_frames_;
    std::shared_ptr<sdp::Frame> current_frame_;
    std::mutex frame_mutex_;
    int32_t max_cache_frames_ = 5;
};

}
#endif
#endif // VIDEORENDERSTUBOPENGL_H
