#ifndef DRM_DISPLAYER_H
#define DRM_DISPLAYER_H

#if defined(__linux__)
#include <inttypes.h>
#include <memory>
#include <mutex>
#include <queue>
#include <list>
#include <map>
#include <string.h>
#include <functional>
#include <condition_variable>

#include <drm/drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "sdpi_basic_declears.h"

struct sp_dev;
struct sp_plane;
struct sp_crtc;
#define DRM_PLANE_TYPE_PRIMARY 1

namespace sdp {


struct DrmFrameBufferCache{
    uint32_t framebuffer_id = 0;
    uint32_t drm_prime_id   = 0;
    uint32_t dumb_id        = 0;
    void*    dumb_addr      = nullptr;
    uint32_t dumb_size      = 0;
    uint16_t width          = 0;
    uint16_t height         = 0;
    uint16_t stride         = 0;
    uint32_t format         = 0;
    uint32_t flags          = 0;
    std::shared_ptr<sdp::Frame> frame;
};

struct PlaneContext{
    uint32_t id                     = 0;
    uint32_t zposPropertyId         = 0;
    uint32_t rotationPropertyId     = 0;
    uint32_t framebufferPropertyId  = 0;
    uint32_t crtcPropertyId         = 0;
    uint32_t srcXPropertyId         = 0;
    uint32_t srcYPropertyId         = 0;
    uint32_t srcWidthPropertyId     = 0;
    uint32_t srcHeightPropertyId    = 0;
    uint32_t crtcXPropertyId        = 0;
    uint32_t crtcYPropertyId        = 0;
    uint32_t crtcWidthPropertyId    = 0;
    uint32_t crtcHeightPropertyId   = 0;

    std::vector<uint32_t> formats;

    int32_t typeValue              = -1;
    int32_t zposValue              = -1;
    int32_t possibleCrtcs      = -1;
    int32_t crtcId                 = -1;
};

struct CrtcContext{
    uint32_t id;
    std::vector<PlaneContext> planes;
};


typedef std::function<int32_t(DrmFrameBufferCache& buffer)> MmapFill;
class VideoRenderStubKms : public ISdpStub {
public:
    VideoRenderStubKms();
    ~VideoRenderStubKms();

    int32_t init(int32_t dri_fd, int32_t crtc_id, int32_t plane_id, const PlaneContext& plane_context);
    int32_t fill_color(bool fill,uint8_t r = 0,uint8_t g = 0,uint8_t b = 0,uint8_t a = 255);
    int32_t render_frame(std::shared_ptr<sdp::Frame> frame);
    int32_t render_image(std::string image);
    void set_all_screen_render(const bool& flag);
    bool all_screen_render() const;
    bool sub_sreen_ready() const;
    void set_subscreen_params(std::vector<std::pair<uint32_t, uint32_t>> params);

    int32_t do_kms_flip(uint16_t h_display, uint16_t v_display, uint16_t rotate, drmModeAtomicReq* request);
    int32_t do_kms_flip_finished();

    // IStub interface
public:
    virtual const std::string &module_name();
    virtual int32_t destory();
private:
    DrmFrameBufferCache create_drm_framebuffer(std::shared_ptr<sdp::Frame> sdp_frame);
    DrmFrameBufferCache create_framebuffer_mmaped(AVFrame *frame);
    void release_framebuffer(DrmFrameBufferCache& framebuffer);
    void initDrmInfo();
    void sub_screen_flip(uint16_t h_display, uint16_t v_display, drmModeAtomicReq *request, uint32_t fb_id, uint32_t fb_w, uint32_t fb_h);
    PlaneContext find_valid_subscreen_param(int fd, uint32_t crtc_id, uint32_t plane_id);
    void fill_framebuffer(DrmFrameBufferCache &bo, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
private:
    std::queue<std::shared_ptr<sdp::Frame>> cache_frames_;
    std::shared_ptr<sdp::Frame> current_frame_;
    std::shared_ptr<sdp::Frame> color_frame_;
    std::mutex      frame_mutex_;
    int32_t         max_cache_frames_       = 2;

    int             decive_dri_fd_          = 0;
    PlaneContext    plane_context_;
    std::map<int32_t,CrtcContext>  crtc_contexts_;
    std::list<PlaneContext> plane_contexts_;

    uint32_t        crtc_id_                = 0;
    uint32_t        plane_id_               = 0;
    uint32_t        sub_crtc_id_            = 0;
    uint32_t        sub_plane_id_           = 0;
    PlaneContext    sub_plane_ctx_;

    DrmFrameBufferCache last_framebuffer_;
    DrmFrameBufferCache release_framebuffer_;
    DrmFrameBufferCache blank_frame_buffer_;
    bool            release_last_framebuffer_;
    int             last_drm_id_            = -1;

    uint8_t         r_                      = 0;
    uint8_t         g_                      = 0;
    uint8_t         b_                      = 0;
    uint8_t         a_                      = 255;
    bool            fill_color_             = false;
    uint64_t        filled_color_           = UINT64_MAX;
    bool            all_screens_render_     = false;
    bool            need_blank_subscreen_   = false;
    bool            sub_screens_ready_      = false;
    std::vector<std::pair<uint32_t, uint32_t>> subscreen_params_;

};
}
#endif //__aarch64__

#endif // DRM_DISPLAYER_H
