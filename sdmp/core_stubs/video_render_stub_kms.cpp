#include "video_render_stub_kms.h"
#include "sdpi_utils.h"
#include <logger.h>
#include <spdlog/fmt/bundled/ranges.h>
#if defined(__linux__)
#include <sys/mman.h>
#include <libdrm/drm.h>
#include <libdrm/drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <iostream>

#include <stb/stb_image.h>
#include <libyuv.h>
extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_drm.h>
#include <libavutil/pixdesc.h>
};

#define CODEC_ALIGN(x, a)   (((x)+(a)-1)&~((a)-1))

int64_t get_plane_property(int fd, int plane_id, const char* name) {
    drmModeObjectProperties* props = drmModeObjectGetProperties(fd, plane_id, DRM_MODE_OBJECT_PLANE);
    drmModePropertyPtr property;
    int i = 0;
    uint32_t id = 0;
    int64_t val = -1;
    for (i = 0; i < props->count_props; i++) {
        property = drmModeGetProperty(fd, props->props[i]);
        if(!strcasecmp(property->name, name)) {
            val = props->prop_values[i];
            id = property->prop_id;
        }
        drmModeFreeProperty(property);
        if(id)
            break;
    }
    drmModeFreeObjectProperties(props);
    return val;
}

uint32_t get_plane_property_id(int fd, int plane_id, const char* name) {
    drmModeObjectProperties* props = drmModeObjectGetProperties(fd, plane_id, DRM_MODE_OBJECT_PLANE);
    drmModePropertyPtr property;
    int i = 0;
    uint32_t id = 0;
    for (i = 0; i < props->count_props; i++) {
        property = drmModeGetProperty(fd, props->props[i]);
        if(!strcasecmp(property->name, name)) {
            id = property->prop_id;            
        }
        drmModeFreeProperty(property);
        if(id)
            break;
    }
    drmModeFreeObjectProperties(props);
    return id;
}

uint32_t get_crtc_property_id(int fd, int crtc_id, const char* name) {
    drmModeObjectProperties* props = drmModeObjectGetProperties(fd, crtc_id, DRM_MODE_OBJECT_CRTC);
    drmModePropertyPtr property;
    int i = 0;
    uint32_t id = 0;
    for (i = 0; i < props->count_props; i++) {
        property = drmModeGetProperty(fd, props->props[i]);
        if(!strcmp(property->name, name)) {
            id = property->prop_id;
        }
        drmModeFreeProperty(property);
        if(id)
            break;
    }
    drmModeFreeObjectProperties(props);
    return id;
}

bool set_plane_property(int fd, uint32_t plane_id, const char* name, const uint64_t& val) {
    drmModeObjectProperties* props = drmModeObjectGetProperties(fd, plane_id, DRM_MODE_OBJECT_PLANE);
    drmModePropertyPtr property;
    int i = 0;
    uint32_t id = 0;
    for (i = 0; i < props->count_props; i++) {
        property = drmModeGetProperty(fd, props->props[i]);
        if(!strcmp(property->name, name))
            id = property->prop_id;
        drmModeFreeProperty(property);
        if(id)
            break;
    }
    drmModeFreeObjectProperties(props);
    if(!id)
        return false;
    int rc = drmModeObjectSetProperty(fd, plane_id, DRM_MODE_OBJECT_PLANE, id, val);
    if(rc)
        std::cerr<< "Failed to set plane property of plane_id: " << plane_id << " name :" << name << " val: " << val;
    return (!rc);
}

int avframe_get_drm_fd(AVFrame* frame){
    AVDRMFrameDescriptor* frame_desc = (AVDRMFrameDescriptor*)frame->data[0];
    int drm_fd = frame_desc->objects[0].fd;
    return drm_fd;
}

int32_t av_frame_get_drm_format(AVFrame* frame){

    switch (frame->format) {
    case AV_PIX_FMT_RGBA: return DRM_FORMAT_ARGB8888;
    case AV_PIX_FMT_BGRA: return DRM_FORMAT_ABGR8888;
    case AV_PIX_FMT_NV12: return DRM_FORMAT_NV12;
    case AV_PIX_FMT_NV21: return DRM_FORMAT_NV21;
    case AV_PIX_FMT_NV16: return DRM_FORMAT_NV16;
    case AV_PIX_FMT_YUV420P: return DRM_FORMAT_YUV420;
    default:
        return 0;
    }
}
int drm_bpp_from_format(uint32_t format, size_t plane)
{
    switch (format) {
    case DRM_FORMAT_BGR233:
    case DRM_FORMAT_C8:
    case DRM_FORMAT_R8:
    case DRM_FORMAT_RGB332:
    case DRM_FORMAT_YVU420:
        return 8;
    /*
     * NV12 is laid out as follows. Each letter represents a byte.
     * Y plane:
     * Y0_0, Y0_1, Y0_2, Y0_3, ..., Y0_N
     * Y1_0, Y1_1, Y1_2, Y1_3, ..., Y1_N
     * ...
     * YM_0, YM_1, YM_2, YM_3, ..., YM_N
     * CbCr plane:
     * Cb01_01, Cr01_01, Cb01_23, Cr01_23, ..., Cb01_(N-1)N, Cr01_(N-1)N
     * Cb23_01, Cr23_01, Cb23_23, Cr23_23, ..., Cb23_(N-1)N, Cr23_(N-1)N
     * ...
     * Cb(M-1)M_01, Cr(M-1)M_01, ..., Cb(M-1)M_(N-1)N, Cr(M-1)M_(N-1)N
     *
     * Pixel (0, 0) requires Y0_0, Cb01_01 and Cr01_01. Pixel (0, 1) requires
     * Y0_1, Cb01_01 and Cr01_01.  So for a single pixel, 2 bytes of luma data
     * are required.
     */
    case DRM_FORMAT_NV12:
        return (plane == 0) ? 8 : 16;
    case DRM_FORMAT_ABGR1555:
    case DRM_FORMAT_ABGR4444:
    case DRM_FORMAT_ARGB1555:
    case DRM_FORMAT_ARGB4444:
    case DRM_FORMAT_BGR565:
    case DRM_FORMAT_BGRA4444:
    case DRM_FORMAT_BGRA5551:
    case DRM_FORMAT_BGRX4444:
    case DRM_FORMAT_BGRX5551:
    case DRM_FORMAT_GR88:
    case DRM_FORMAT_RG88:
    case DRM_FORMAT_RGB565:
    case DRM_FORMAT_RGBA4444:
    case DRM_FORMAT_RGBA5551:
    case DRM_FORMAT_RGBX4444:
    case DRM_FORMAT_RGBX5551:
    case DRM_FORMAT_UYVY:
    case DRM_FORMAT_VYUY:
    case DRM_FORMAT_XBGR1555:
    case DRM_FORMAT_XBGR4444:
    case DRM_FORMAT_XRGB1555:
    case DRM_FORMAT_XRGB4444:
    case DRM_FORMAT_YUYV:
    case DRM_FORMAT_YVYU:
        return 16;
    case DRM_FORMAT_BGR888:
    case DRM_FORMAT_RGB888:
        return 24;
    case DRM_FORMAT_ABGR2101010:
    case DRM_FORMAT_ABGR8888:
    case DRM_FORMAT_ARGB2101010:
    case DRM_FORMAT_ARGB8888:
    case DRM_FORMAT_AYUV:
    case DRM_FORMAT_BGRA1010102:
    case DRM_FORMAT_BGRA8888:
    case DRM_FORMAT_BGRX1010102:
    case DRM_FORMAT_BGRX8888:
    case DRM_FORMAT_RGBA1010102:
    case DRM_FORMAT_RGBA8888:
    case DRM_FORMAT_RGBX1010102:
    case DRM_FORMAT_RGBX8888:
    case DRM_FORMAT_XBGR2101010:
    case DRM_FORMAT_XBGR8888:
    case DRM_FORMAT_XRGB2101010:
    case DRM_FORMAT_XRGB8888:
        return 32;
    }
    fprintf(stderr, "drv: UNKNOWN FORMAT %d\n", format);
    return 0;
}

namespace sdp {

VideoRenderStubKms::VideoRenderStubKms()
{
}

VideoRenderStubKms::~VideoRenderStubKms()
{
    //    release_framebuffer(last_framebuffer_);
}

int32_t VideoRenderStubKms::init(int32_t dri_fd,int32_t crtc_id, int32_t plane_id, const PlaneContext &plane_context)
{
    if(crtc_id_)
        return 0;
    crtc_id_ = crtc_id;
    plane_id_ = plane_id;
    plane_context_ = plane_context;
    decive_dri_fd_ = dri_fd;
    initDrmInfo();
    return 0;
}

int32_t VideoRenderStubKms::fill_color(bool fill, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    fill_color_ = fill;
    if(fill_color_ == false)
        filled_color_ = UINT64_MAX;

    r_ = r;
    g_ = g;
    b_ = b;
    a_ = a;
    return 0;
}

int32_t VideoRenderStubKms::render_frame(std::shared_ptr<sdp::Frame> frame)
{
    if(frame->frame->format == AV_PIX_FMT_YUV420P || frame->frame->format == AV_PIX_FMT_YUVJ420P){
        AVFrame* new_frame = (AVFrame*)malloc(sizeof(AVFrame));
        memset(new_frame,0,sizeof (AVFrame));
        new_frame->width = frame->frame->width;
        new_frame->height = frame->frame->height;
//        new_frame->format = AV_PIX_FMT_NV12;
//        new_frame->linesize[0] = frame->frame->width;
//        new_frame->linesize[1] = frame->frame->width;
//        new_frame->linesize[2] = new_frame->linesize[3] = 0;
//        //av_frame_get_buffer(nv12_frame,1);
//        uint8_t *nv12_buffer = (uint8_t*)malloc(new_frame->width*new_frame->height*3/2);
//        new_frame->data[0] = nv12_buffer;
//        new_frame->data[1] = nv12_buffer + (new_frame->width*new_frame->height);
//        new_frame->data[2] = new_frame->data[3] = nullptr;
//        libyuv::I420ToNV12(frame->frame->data[0],frame->frame->linesize[0],
//                frame->frame->data[1],frame->frame->linesize[1],
//                frame->frame->data[2],frame->frame->linesize[2],
//                new_frame->data[0],new_frame->linesize[0],
//                new_frame->data[1],new_frame->linesize[1],
//                frame->frame->width,frame->frame->height);

        MP_WARN("KMS-RENDER USE MMAP ,CONVERT {} to RGBA",av_get_pix_fmt_name((AVPixelFormat)frame->frame->format));

        new_frame->format = AV_PIX_FMT_RGBA;
        new_frame->linesize[0] = frame->frame->width*4;
        new_frame->linesize[1] = new_frame->linesize[2] = new_frame->linesize[3] = 0;
        uint8_t *rgba_buffer = (uint8_t*)malloc(new_frame->width*new_frame->height*4);
        new_frame->data[0] = rgba_buffer;
        new_frame->data[1] = new_frame->data[2] = new_frame->data[3] = nullptr;
        libyuv::I420ToARGB(frame->frame->data[0],frame->frame->linesize[0],
                        frame->frame->data[1],frame->frame->linesize[1],
                        frame->frame->data[2],frame->frame->linesize[2],
                            rgba_buffer,frame->frame->width*4,
                            frame->frame->width,frame->frame->height);
        frame = sdp::Frame::make_frame(new_frame);
        frame->releaser = [](AVFrame* frame,AVPacket*){
            free(frame->data[0]);
            free(frame);
        };
    }

    std::lock_guard<std::mutex> lock(frame_mutex_);
    if(cache_frames_.size() > max_cache_frames_){
        cache_frames_.pop();
    }
    if(plane_context_.crtcPropertyId){
        cache_frames_.push(frame);
    }    
    return  0;
}

int32_t VideoRenderStubKms::render_image(std::string image)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    void* image_buffer = stbi_load(image.c_str(),&width,&height,&channels,4);
    if(image_buffer == nullptr){
        MP_ERROR("{} failed can't load image:{}",__FUNCTION__,image);
        return -1;
    }

    MP_INFO("{} load image:{} successed:{}x{}",__FUNCTION__,image,width,height);

    AVFrame* new_frame = (AVFrame*)malloc(sizeof(AVFrame));
    memset(new_frame,0,sizeof (AVFrame));
    new_frame->width = width;
    new_frame->height = height;
    new_frame->format = AV_PIX_FMT_RGBA;
    new_frame->linesize[0] = width*4;
    new_frame->linesize[1] = new_frame->linesize[2] = new_frame->linesize[3] = 0;
    uint8_t *rgba_buffer = (uint8_t*)malloc(new_frame->width*new_frame->height*4);
    new_frame->data[0] = rgba_buffer;
    new_frame->data[1] = new_frame->data[2] = new_frame->data[3] = nullptr;

    memcpy(rgba_buffer,image_buffer,width*height*4);
    stbi_image_free(image_buffer);

    auto frame = sdp::Frame::make_frame(new_frame);
    frame->releaser = [](AVFrame* frame,AVPacket*){
        free(frame->data[0]);
        free(frame);
    };
    render_frame(frame);
    return 0;
}

void VideoRenderStubKms::set_all_screen_render(const bool &flag)
{
    all_screens_render_ = flag;
    if(!flag)
        need_blank_subscreen_ = true;
}

bool VideoRenderStubKms::all_screen_render() const
{
    return all_screens_render_;
}

bool VideoRenderStubKms::sub_sreen_ready() const
{
    return sub_screens_ready_;
}

void VideoRenderStubKms::set_subscreen_params(std::vector<std::pair<uint32_t, uint32_t>> params)
{
    subscreen_params_ = params;
}

int32_t VideoRenderStubKms::do_kms_flip(uint16_t h_display, uint16_t v_display, uint16_t rotate, drmModeAtomicReq *request)
{
//    static FpsCalc fps;
//    fps.frame();
    std::lock_guard<std::mutex> lock(frame_mutex_);

    if(plane_context_.crtcPropertyId == 0 || plane_context_.id == 0){
        MP_WARN("VideoRenderStubKms::do_kms_flip : plane maybe not initialized currently, skip this frame...");
        return 0;
    }
    bool need_refresh = fill_color_;
    if(cache_frames_.size()){
        need_refresh = true;
        current_frame_ = cache_frames_.front();
        cache_frames_.pop();
    }
    DrmFrameBufferCache buffer;
    if(need_refresh){
        int framebuffer_id = 0;
        if(fill_color_){
            //current not implement memory buffer mmap,just only fill color
            last_drm_id_ = 0;
            uint32_t code = fourcc_code(r_,g_,b_,a_);
            if(filled_color_ != code){
                filled_color_ = code;
                color_frame_ = sdp::sdp_frame_make_color_frame(AV_PIX_FMT_RGBA,360,180,r_,g_,b_,a_);
                buffer = create_framebuffer_mmaped(color_frame_->frame);
            }
            else {
                return 0;
            }
        }
        else if(current_frame_->frame->format == AV_PIX_FMT_DRM_PRIME){
            filled_color_ = UINT64_MAX;
            int currend_frame_drm_fd = avframe_get_drm_fd(current_frame_->frame);
            if(last_drm_id_ != currend_frame_drm_fd){
                buffer = create_drm_framebuffer(current_frame_);
                if(buffer.framebuffer_id == 0)
                    return -1;
                last_drm_id_ = currend_frame_drm_fd;
            }
            else {
                return 0;
            }

        }
        else{
            buffer = create_framebuffer_mmaped(current_frame_->frame);
        }

        if(release_last_framebuffer_){
            release_framebuffer(release_framebuffer_);
        }

        int ret = 0;

        framebuffer_id = buffer.framebuffer_id;
        if(request){
            drmModeAtomicAddProperty(request, plane_context_.id, plane_context_.framebufferPropertyId, framebuffer_id);
            drmModeAtomicAddProperty(request, plane_context_.id, plane_context_.crtcPropertyId, crtc_id_);

            drmModeAtomicAddProperty(request, plane_context_.id, plane_context_.srcXPropertyId, 0);
            drmModeAtomicAddProperty(request, plane_context_.id, plane_context_.srcYPropertyId, 0);
            drmModeAtomicAddProperty(request, plane_context_.id, plane_context_.srcWidthPropertyId,
                                     uint64_t(buffer.width) << 16);
            drmModeAtomicAddProperty(request, plane_context_.id, plane_context_.srcHeightPropertyId,
                                     uint64_t(buffer.height) << 16);
            drmModeAtomicAddProperty(request, plane_context_.id, plane_context_.crtcXPropertyId, 0);
            drmModeAtomicAddProperty(request, plane_context_.id, plane_context_.crtcYPropertyId, 0);
            drmModeAtomicAddProperty(request, plane_context_.id, plane_context_.crtcWidthPropertyId,h_display);
            drmModeAtomicAddProperty(request, plane_context_.id, plane_context_.crtcHeightPropertyId,v_display);

            if(plane_context_.rotationPropertyId){
                int rotate_prop_value = DRM_MODE_ROTATE_0;
                switch (rotate) {
                    case 0:rotate_prop_value = DRM_MODE_ROTATE_0;break;
                    case 90:rotate_prop_value = DRM_MODE_ROTATE_90;break;
                    case 180:rotate_prop_value = DRM_MODE_ROTATE_180;break;
                    case 270:rotate_prop_value = DRM_MODE_ROTATE_270;break;
                    default:rotate_prop_value = DRM_MODE_ROTATE_0;
                }
                if(fill_color_ || current_frame_->frame->format != AV_PIX_FMT_DRM_PRIME)
                    rotate_prop_value = DRM_MODE_ROTATE_0;
                drmModeAtomicAddProperty(request, plane_context_.id, plane_context_.rotationPropertyId,rotate_prop_value);
            }

            // for subscreen render
            if(all_screens_render_)
                sub_screen_flip(h_display,v_display, request, buffer.framebuffer_id, buffer.width, buffer.height);
            else if(need_blank_subscreen_) {
                sub_screen_flip(h_display,v_display, request, blank_frame_buffer_.framebuffer_id, blank_frame_buffer_.width, blank_frame_buffer_.height);
                need_blank_subscreen_ = false;
            }

        }else {
            ret = drmModeSetPlane(decive_dri_fd_, plane_context_.id,
                                  crtc_id_, framebuffer_id, 0, 0, 0,
                                  h_display,
                                  v_display,
                                  0, 0, buffer.width << 16, buffer.height << 16);
            // not support no atomic modeset for drm currently.
//            if(all_screens_render_) {
//                foreach (const auto& ctx, crtc_contexts_) {
//                    if(ctx.id == select_crtc_id_)
//                        continue;
//                    if(ctx.planes.size() <= 0)
//                        continue;
//                    const auto& planeCtx = ctx.planes.first();
//                    set_plane_property(drm_device_fd_, planeCtx.id, "ZPOS", 4);
//                    drmModeSetPlane(drm_device_fd_, planeCtx.id,
//                                                      ctx.id, framebuffer_id, 0, 0, 0,
//                                                      output_->modes[output_->mode].hdisplay,
//                                                      output_->modes[output_->mode].vdisplay,
//                                                      0, 0, buffer.width << 16, buffer.height << 16);
//                }
//            }
        }

        release_last_framebuffer_ = (last_framebuffer_.framebuffer_id != buffer.framebuffer_id);
        if(release_last_framebuffer_)
            release_framebuffer_ = last_framebuffer_;
        last_framebuffer_ = buffer;
//        MP_INFO("render frame: device_fd:{} crtc:{} plane:{} format:{} framebuffer:{}  pts:{} ret:{} ",
//                    decive_dri_fd_,crtc_id_,plane_id_,
//                    current_frame_->frame->format,framebuffer_id,current_frame_->frame->pts , ret);
    }

    return 0;
}

int32_t VideoRenderStubKms::do_kms_flip_finished()
{

    if(release_last_framebuffer_){
       // release_framebuffer(last_framebuffer_);
    }
    return 0;
}

DrmFrameBufferCache VideoRenderStubKms::create_drm_framebuffer(std::shared_ptr<sdp::Frame> sdp_frame)
{    
    DrmFrameBufferCache bo;
    AVFrame* frame = sdp_frame->frame;
    if(frame->format != AV_PIX_FMT_DRM_PRIME)
        return bo;

    uint32_t handles[4]={0}, pitches[4]={0}, offsets[4]={0},flags = 0;
    uint64_t modifier[4]={0};
    int frm_size, ret, err;

    AVDRMFrameDescriptor* frame_desc = (AVDRMFrameDescriptor*)frame->data[0];

    int drm_fd = avframe_get_drm_fd(frame);

    int width = CODEC_ALIGN(frame->width, 16);
    int height = CODEC_ALIGN(frame->height, 16);
    frm_size = width * height * 3 / 2;

    drmPrimeFDToHandle(decive_dri_fd_, drm_fd, &bo.drm_prime_id);
    bo.width = width;
    bo.height = height;
    bo.format = DRM_FORMAT_NV12;
    bo.flags = 0;

    int32_t drm_plane_index = 0;
    for (int layer_index = 0; layer_index < frame_desc->nb_layers; layer_index++){
        AVDRMLayerDescriptor* layer = &frame_desc->layers[layer_index];
        for (int plane_index = 0; plane_index < layer->nb_planes; plane_index++)
        {
            int object = layer->planes[plane_index].object_index;
            uint32_t handle = bo.drm_prime_id;//current one handle;
            if (handle && layer->planes[plane_index].pitch)
            {
                handles[drm_plane_index] = handle;
                pitches[drm_plane_index] = layer->planes[plane_index].pitch;
                offsets[drm_plane_index] = layer->planes[plane_index].offset;
                modifier[drm_plane_index] = frame_desc->objects[object].format_modifier;
                //MP_INFO("drm layer_index:{} layer-plane:{} drm_plane_index:{} handle:{} pitch:{} offset:{}",layer_index,plane_index,drm_plane_index,handle,layer->planes[plane_index].pitch,layer->planes[plane_index].offset);
                drm_plane_index++;
            }
        }
    }


    if (modifier[0] && modifier[0] != DRM_FORMAT_MOD_NONE)
        flags = DRM_MODE_FB_MODIFIERS;

    // add the video frame FB
    ret = drmModeAddFB2WithModifiers(decive_dri_fd_, bo.width, bo.height, bo.format,
                                   handles, pitches, offsets, modifier, &bo.framebuffer_id, flags);

    if(ret < 0){
        MP_ERROR("{} drmModeAddFB2WithModifiers layers:{} frame_fd:{} size:{}x{} ret:{}" ,__FUNCTION__,frame_desc->nb_layers, bo.drm_prime_id,frame->width,frame->height,ret);
    }
    bo.frame = sdp_frame;
    return bo;
}

DrmFrameBufferCache VideoRenderStubKms::create_framebuffer_mmaped(AVFrame* frame )
{
    struct drm_mode_create_dumb cd;

    int drm_format = av_frame_get_drm_format(frame);

    memset(&cd, 0, sizeof(cd));
    cd.width = frame->width;
    cd.height = frame->height;
    cd.bpp = drm_bpp_from_format(drm_format,0);//argb
    cd.flags = 0;

    DrmFrameBufferCache bo;
    int ret = drmIoctl(decive_dri_fd_, DRM_IOCTL_MODE_CREATE_DUMB, &cd);
    if (ret) {
        MP_LOG_DEAULT("failed to create dri:{} dumb {} ",decive_dri_fd_, ret);
        return bo;
    }

    bo.width = cd.width;
    bo.height = cd.height;
    bo.format = drm_format;
    bo.dumb_size = cd.size;
    bo.dumb_id = cd.handle;
    bo.stride = cd.pitch;
    bo.flags = cd.flags;

    std::vector<uint32_t> handles(4,0), pitches(4,0), offsets(4,0);
    int offset = 0;
    for(int index = 0; index < 4; index++){
        if(frame->data[index] == 0)
            break;
        handles[index] = bo.dumb_id;
        pitches[index] = bo.stride;
        offsets[index] = offset;
        offset += bo.stride*bo.height;
    }

    ret = drmModeAddFB2(decive_dri_fd_, bo.width, bo.height,
                        drm_format, handles.data(), pitches.data(), offsets.data(),
                        &bo.framebuffer_id, bo.flags);
    if (ret) {
        MP_ERROR("failed to create sp_bo ret:{} dri_id:{} format:{} handles:{} pitches:{} offsets:{} bo_size:{}x{} bo_stride:{}",
                       ret,decive_dri_fd_,sdp::StringUtils::fourcc(drm_format),
                       handles,pitches,offsets,
                       bo.width, bo.height,bo.stride);
        //abort();
        return bo;
    }

    struct drm_mode_map_dumb md;

    md.handle = bo.dumb_id;
    ret = drmIoctl(decive_dri_fd_, DRM_IOCTL_MODE_MAP_DUMB, &md);
    if (ret) {
        MP_LOG_DEAULT("failed to map sp_bo ret={}", ret);
        return bo;
    }

    bo.dumb_addr = mmap(NULL, bo.dumb_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                        decive_dri_fd_, md.offset);
    if (bo.dumb_addr == MAP_FAILED) {
        MP_LOG_DEAULT("failed to map bo ret={}", -errno);
        return bo;
    }

    AVFrame dest={0};
    dest.width = frame->width;
    dest.height = frame->height;
    dest.format = frame->format;
    for(int index = 0; index < 4; index++){
        dest.data[index] = (uint8_t*)bo.dumb_addr + offsets[index];
        dest.linesize[index] = pitches[index];
    }
    ret = av_frame_copy(&dest,frame);

//    if (bo.dumb_addr)
//        munmap(bo.dumb_addr,bo.dumb_size);    
    return bo;
}

void VideoRenderStubKms::release_framebuffer(DrmFrameBufferCache& framebuffer)
{
    if (framebuffer.dumb_addr)
        munmap(framebuffer.dumb_addr,framebuffer.dumb_size);

    if(framebuffer.framebuffer_id){
        drmModeRmFB(decive_dri_fd_, framebuffer.framebuffer_id);
    }
    if (framebuffer.drm_prime_id) {
        struct drm_gem_close req;
        req.handle = framebuffer.drm_prime_id;
        drmIoctl(decive_dri_fd_, DRM_IOCTL_GEM_CLOSE, &req);
    }

    if (framebuffer.dumb_id) {
        struct drm_mode_destroy_dumb dd;
        dd.handle = framebuffer.dumb_id;
        drmIoctl(decive_dri_fd_, DRM_IOCTL_MODE_DESTROY_DUMB, &dd);
    }

    framebuffer.framebuffer_id = 0;
    framebuffer.drm_prime_id = 0;
    framebuffer.dumb_addr = nullptr;
    framebuffer.dumb_id = 0;
}

void VideoRenderStubKms::initDrmInfo()
{
    crtc_contexts_.clear();

    int fd = decive_dri_fd_;
    MP_LOG_DEAULT("#initialziing drm_fd:{} info... ", fd);

    drmModeConnector *conn = nullptr;
    drmModeRes *res = nullptr;
    drmModePlaneRes *plane_res = nullptr;
    res = drmModeGetResources(fd);
    plane_res = drmModeGetPlaneResources(fd);

    for(int crtc_index = 0; crtc_index < res->count_crtcs; crtc_index++) {
        CrtcContext crtc_context;
        crtc_context.id = res->crtcs[crtc_index];
        uint32_t crtc_mask = 1<<crtc_index;
        crtc_contexts_[crtc_mask] = crtc_context;
    }

    const char* type_strings[3] = {"overlay","primary","cursor "};

    for(int plane_index = 0; plane_index < plane_res->count_planes; plane_index++) {
        auto plane_id = plane_res->planes[plane_index];
        drmModePlanePtr plane = drmModeGetPlane(fd,plane_id);

        uint32_t crtc_mask = 0;

        PlaneContext plane_context;
        plane_context.id = plane_id;
        plane_context.zposPropertyId = get_plane_property_id(fd, plane_id, "ZPOS");
        plane_context.framebufferPropertyId = get_plane_property_id(fd, plane_id, "FB_ID");
        plane_context.crtcPropertyId = get_plane_property_id(fd, plane_id, "CRTC_ID");
        plane_context.srcXPropertyId = get_plane_property_id(fd, plane_id, "SRC_X");
        plane_context.srcYPropertyId = get_plane_property_id(fd, plane_id, "SRC_Y");
        plane_context.srcWidthPropertyId = get_plane_property_id(fd, plane_id, "SRC_W");
        plane_context.srcHeightPropertyId = get_plane_property_id(fd, plane_id, "SRC_H");
        plane_context.crtcXPropertyId = get_plane_property_id(fd, plane_id, "CRTC_X");
        plane_context.crtcYPropertyId = get_plane_property_id(fd, plane_id, "CRTC_Y");
        plane_context.crtcWidthPropertyId = get_plane_property_id(fd, plane_id, "CRTC_W");
        plane_context.crtcHeightPropertyId = get_plane_property_id(fd, plane_id, "CRTC_H");
        plane_context.rotationPropertyId = get_plane_property_id(fd, plane_id, "ROTATION");

        plane_context.possibleCrtcs = plane->possible_crtcs;
        for(uint32_t format_index = 0; format_index < plane->count_formats; format_index++){
            plane_context.formats.push_back(plane->formats[format_index]);
        }

        plane_context.typeValue = get_plane_property(fd, plane_id, "TYPE");
        plane_context.zposValue = get_plane_property(fd, plane_id, "ZPOS");

        if(plane_context.zposValue == -1){
            if(plane_context.typeValue == DRM_PLANE_TYPE_PRIMARY)
                plane_context.zposValue = 0;
            else if(plane_context.typeValue == DRM_PLANE_TYPE_OVERLAY)
                plane_context.zposValue = 1;
            else if(plane_context.typeValue == DRM_PLANE_TYPE_CURSOR)
                plane_context.zposValue = 2;
            MP_WARN("# drm info get logic zpos:{} of plane:{} type:{}",plane_context.zposValue,plane_context.id,type_strings[plane_context.typeValue]);
        }

        auto crtc_it =crtc_contexts_.find(plane_context.possibleCrtcs);
        if(crtc_it != crtc_contexts_.end()){
            plane_context.crtcId = crtc_it->second.id;
            crtc_it->second.planes.push_back(plane_context);
        }
        plane_contexts_.push_back(plane_context);

        drmModeFreePlane(plane);
    }

    MP_INFO("# drm info finished crtcs count:{}",crtc_contexts_.size());
    for(const auto& crtc_context : crtc_contexts_) {
        MP_INFO("# crtc:{}  planes count:{}",crtc_context.second.id ,crtc_context.second.planes.size());
        for(auto& item : crtc_context.second.planes){
            std::string formats_str;
            for(auto item : item.formats){
                formats_str += sdp::StringUtils::fourcc(item) + " ";
            }
            MP_INFO("#\tplane:{} type:{} crtc:{}={} z:{} formats: {}",item.id,type_strings[item.typeValue],item.possibleCrtcs,item.crtcId,item.zposValue,formats_str);
        }
    }

   auto color_frame = sdp::sdp_frame_make_color_frame(AV_PIX_FMT_RGBA,320,180,0,0,0,0);
   blank_frame_buffer_ = create_framebuffer_mmaped(color_frame->frame);

    drmModeFreePlaneResources(plane_res);
    drmModeFreeResources(res);
    sub_screens_ready_ = true;
}

void VideoRenderStubKms::sub_screen_flip(uint16_t h_display, uint16_t v_display, drmModeAtomicReq *request, uint32_t fb_id, uint32_t fb_w, uint32_t fb_h)
{
    for(auto it: subscreen_params_) {
        auto crtc_id = it.first;
        auto plane_id = it.second;
        auto pctx = find_valid_subscreen_param(decive_dri_fd_, crtc_id, plane_id);
        if(pctx.id <= 0) {
            std::cerr << "no available plane found of " << crtc_id << plane_id << ", skipping";
            continue;
        }

        drmModeAtomicAddProperty(request, plane_id, pctx.framebufferPropertyId, fb_id);
        drmModeAtomicAddProperty(request, plane_id, pctx.crtcPropertyId, crtc_id);
        drmModeAtomicAddProperty(request, plane_id, pctx.srcXPropertyId, 0);
        drmModeAtomicAddProperty(request, plane_id, pctx.srcYPropertyId, 0);
        drmModeAtomicAddProperty(request, plane_id, pctx.srcWidthPropertyId,
                                 fb_w << 16);
        drmModeAtomicAddProperty(request, plane_id, pctx.srcHeightPropertyId,
                                 fb_h << 16);
        drmModeAtomicAddProperty(request, plane_id, pctx.crtcXPropertyId, 0);
        drmModeAtomicAddProperty(request, plane_id, pctx.crtcYPropertyId, 0);
        drmModeAtomicAddProperty(request, plane_id, pctx.crtcWidthPropertyId,h_display);
        drmModeAtomicAddProperty(request, plane_id, pctx.crtcHeightPropertyId,v_display);
    }
}

PlaneContext VideoRenderStubKms::find_valid_subscreen_param(int fd, uint32_t crtc_id, uint32_t plane_id)
{
    PlaneContext pctx;
    for(const auto& context: plane_contexts_) {
        if(context.id == plane_id) {
            pctx = context;
            break;
        }
    }
    return pctx;
}

const std::string &VideoRenderStubKms::module_name()
{
    static const std::string id = "VideoRenderStubKms";
    return id;
}

int32_t VideoRenderStubKms::destory()
{
    delete this;
    return 0;
}

}
#endif
