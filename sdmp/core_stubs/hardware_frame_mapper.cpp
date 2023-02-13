#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#include "hardware_frame_mapper.h"

#if defined(__linux__)
#include <libdrm/drm_fourcc.h>
extern "C"{
#include <libavutil/hwcontext_drm.h>
#include <libavutil/hwcontext_vaapi.h>
}
#endif

#if defined (HAS_VAAPI)
#include <X11/Xlib.h>
#include <va/va_x11.h>
#include <va/va_egl.h>
#include <va/va_drmcommon.h>
#endif

extern "C"{
#include <libavutil/frame.h>
#include <libavutil/hwcontext.h>
}

#include <logger.h>

int32_t check_gl_error(const char *cls, const char *msg) {
    GLenum err = GL_NO_ERROR;
    int32_t err_has = GL_NO_ERROR;
    do{
        err = glGetError();
        if (err != GL_NO_ERROR) {
            MP_LOG_DEAULT( "[{}] {}: GL error '{}' occured\n",cls, msg, err);
            err_has = err;
        }
    }while (err!=GL_NO_ERROR);
    return err_has;
}
#define CHECK_GL_ERROR(function) check_gl_error(__FUNCTION__, function);

using namespace sdp;

HardwareFrameMapper::HardwareFrameMapper()
{

}

HardwareFrameMapper::~HardwareFrameMapper()
{
#if defined (HAS_VAAPI_GLX)
    if (!x11_pixelmap_) {
        XFreePixmap(glx_display_, x11_pixelmap_);
        x11_pixelmap_ = 0;
    }
    if(glx_pixmap_){
        glXDestroyGLXPixmap(glx_display_,glx_pixmap_);
    }
    for(auto item : glx_surfaces_){
        VaapiHelper::destroy_glx_surface(&item.second);
    }
#endif

#if defined(__linux__)
    if(egl_image_ != nullptr){
        eglDestroyImageKHR(eglGetCurrentDisplay(),egl_image_);
        CHECK_GL_ERROR("eglDestroyImageKHR");
    }
#endif
}

int32_t HardwareFrameMapper::create_context(DisplayType display_type)
{
    display_type_ = display_type;
//    if(display_type == kDisplayTypeX11){
//        native_display_handle_ = (uintptr_t)XOpenDisplay(NULL);
//        int ret = XInitThreads();
//        if(ret == 0){
//            return -1;
//        }
//        display_handle_ =  vaGetDisplay((Display*)native_display_handle_);
//    }
//    else if(display_type == kDisplayTypeGLX){
//        native_display_handle_ = (uintptr_t)XOpenDisplay(NULL);
//        int ret = XInitThreads();
//        if(ret == 0){
//            return -1;
//        }
//        //display_handle_ =  vaGetDisplayGLX((Display*)native_display_handle_);
//    }

//    auto status  = vaInitialize(display_handle_,&major_version_,&minor_version_);
//    if(status != VA_STATUS_SUCCESS)
//        return -status;

    return 0;
}
int32_t HardwareFrameMapper::map_hardware_frame_to_texture(const AVFrame *frame, int texture)
{
    if(frame->format == AV_PIX_FMT_DRM_PRIME){
        int32_t ret = map_drm_frame_to_egl_texture(frame,texture);
        if(ret < 0){
            return kErrorFallbackToSoftwareFrame;
        }
    }
#if defined (HAS_VAAPI)
    else if(frame->format == AV_PIX_FMT_VAAPI){
        if(frame->format != AV_PIX_FMT_VAAPI)
            return -1;
        auto ctx = (AVHWFramesContext*)frame->hw_frames_ctx->data;
        auto surface_id = (VASurfaceID)(uintptr_t)frame->data[3];
        AVVAAPIDeviceContext *hwctx = (AVVAAPIDeviceContext*)ctx->device_ctx->hwctx;
        VASurfaceID va_surface = (VASurfaceID)(uintptr_t)frame->data[3];

        if(display_type_ == kDisplayTypeEGL){
                    return map_vaapi_frame_to_egl_texture(hwctx->display, surface_id,texture);
        }
#if defined (HAS_VAAPI_GLX)
        else if(display_type_ == kDisplayTypeX11){
            return map_vaapi_frame_to_x11_texture(hwctx->display, surface_id,texture,frame->width,frame->height);
        }
        else if(display_type_ == kDisplayTypeGLX){
            //return map_vaapi_frame_to_glx_texture(hwctx->display, surface_id,texture);
            //current use x11, ffmpeg only open drm or x11
            return map_vaapi_frame_to_x11_texture(hwctx->display, surface_id,texture,frame->width,frame->height);
        }
#endif//HAS_VAAPI_GLX
    }
#endif//HAS_VAAPI

    return -1;
}


int32_t HardwareFrameMapper::map_drm_frame_to_egl_texture(const AVFrame *frame, int texture)
{
#if defined(GL_TEXTURE_EXTERNAL_OES) && defined (EGL_LINUX_DMA_BUF_EXT) && !defined (_WIN32)
    //TIMER_DECLEAR(timer)
    AVDRMFrameDescriptor* frame_desc = (AVDRMFrameDescriptor*)frame->data[0];
    int stride[3] = {0};
    int offset[3] = {0};
    stride[0] = stride[1] = frame_desc->layers[0].planes[0].pitch;
    offset[1] = frame_desc->layers[0].planes[1].offset;
    int drm_fd = frame_desc->objects[0].fd;
    const int drm_format = DRM_FORMAT_NV12;
    EGLint __attr[] = {
        EGL_WIDTH,			frame->width,
        EGL_HEIGHT,			frame->height,
        EGL_LINUX_DRM_FOURCC_EXT,	drm_format,
        EGL_DMA_BUF_PLANE0_FD_EXT,	drm_fd,
        EGL_DMA_BUF_PLANE0_OFFSET_EXT,	offset[0],
        EGL_DMA_BUF_PLANE0_PITCH_EXT,	stride[0],
        EGL_DMA_BUF_PLANE1_FD_EXT,	drm_fd,
        EGL_DMA_BUF_PLANE1_OFFSET_EXT,	offset[1],
        EGL_DMA_BUF_PLANE1_PITCH_EXT,	stride[1],
        EGL_NONE
    };
    if(egl_image_ != nullptr){
        eglDestroyImageKHR(eglGetCurrentDisplay(),egl_image_);
        CHECK_GL_ERROR("eglDestroyImageKHR");
    }
    egl_image_ = eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, 0, __attr);
    CHECK_GL_ERROR("eglCreateImageKHR");

//    MP_LOG_DEAULT("DRM FRAME:{} eglImage:{} pts:{} width:{} height:{} stride:{}.{}.{} offset:{}.{}.{} drm_fd:{}\n",
//        drm_fd,
//        egl_image_,
//        (int)frame->pts,
//        frame->width,
//        frame->height,
//        stride[0],stride[1],stride[2],
//        offset[0],offset[1],offset[2],
//        drm_fd);

    glActiveTexture( GL_TEXTURE1);
    CHECK_GL_ERROR("glActiveTexture");

    glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
    glGetError();//MARK:RK3288 will 1282, but it's work correctlly,emm~~~.use glGetError to pop errors

    glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, egl_image_);
    CHECK_GL_ERROR("glEGLImageTargetTexture2DOES");

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    CHECK_GL_ERROR("glTexParameteri");

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    CHECK_GL_ERROR("glTexParameteri");
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    CHECK_GL_ERROR("glTexParameteri");
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    CHECK_GL_ERROR("glTexParameteri");
    //TIMER_STOP(timer)
    //MP_LOG_DEAULT("map drm frame {} to texture:{} use %f ms\n",drm_fd,texture_id,timer.get_elapsed_millisecond());
    return 0;
#else
    return -1;
#endif
}
#if defined (HAS_VAAPI)
int32_t HardwareFrameMapper::derive_image(VADisplay display, VASurfaceID surface)
{
    if (va_image.buf != VA_INVALID_ID) {
        vaReleaseBufferHandle(display, va_image.buf);
        va_image.buf = VA_INVALID_ID;
    }
    if (va_image.image_id != VA_INVALID_ID) {
        vaDestroyImage(display, va_image.image_id);
        va_image.image_id = VA_INVALID_ID;
    }

    VAStatus status = VA_STATUS_SUCCESS;
    status = vaSyncSurface(display, surface);
    if(status != VA_STATUS_SUCCESS)
        return -status;
    status = vaDeriveImage(display, surface, &va_image);
    if(status != VA_STATUS_SUCCESS)
        return -status;

    va_buffer_info.mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
    status = vaAcquireBufferHandle(display, va_image.buf, &va_buffer_info);
    if(status != VA_STATUS_SUCCESS)
        return -status;

    return status;
}
int32_t HardwareFrameMapper::map_vaapi_frame_to_egl_texture(VADisplay display, VASurfaceID surface, int texture)
{
    derive_image(display,surface);

    const int drm_format = DRM_FORMAT_NV12;
    EGLint __attr[] = {
        EGL_WIDTH,                      (int)va_image.width,
        EGL_HEIGHT,                     (int)va_image.height,
        EGL_LINUX_DRM_FOURCC_EXT,       (int)drm_format,
        EGL_DMA_BUF_PLANE0_FD_EXT,      (int)va_buffer_info.handle,
        EGL_DMA_BUF_PLANE0_OFFSET_EXT,	(int)va_image.offsets[0],
        EGL_DMA_BUF_PLANE0_PITCH_EXT,	(int)va_image.pitches[0],
        EGL_DMA_BUF_PLANE1_FD_EXT,      (int)va_buffer_info.handle,
        EGL_DMA_BUF_PLANE1_OFFSET_EXT,	(int)va_image.offsets[1],
        EGL_DMA_BUF_PLANE1_PITCH_EXT,	(int)va_image.pitches[1],
        EGL_NONE
    };
    if(egl_image_ != nullptr){
        eglDestroyImageKHR(eglGetCurrentDisplay(),egl_image_);
        CHECK_GL_ERROR("eglDestroyImageKHR");
    }
    egl_image_ = (EGLImage*)eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, 0, __attr);
    CHECK_GL_ERROR("eglCreateImageKHR");

    glActiveTexture( GL_TEXTURE1);
    CHECK_GL_ERROR("glActiveTexture");

    glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
    glGetError();//MARK:RK3288 will 1282, but it's work correctlly,emm~~~.use glGetError to pop errors

    glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, egl_image_);
    CHECK_GL_ERROR("glEGLImageTargetTexture2DOES");

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    CHECK_GL_ERROR("glTexParameteri");
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    CHECK_GL_ERROR("glTexParameteri");
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    CHECK_GL_ERROR("glTexParameteri");
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    CHECK_GL_ERROR("glTexParameteri");
    return 0;
}

#if defined (HAS_VAAPI_GLX)
int32_t HardwareFrameMapper::map_vaapi_frame_to_glx_texture(VADisplay display, VASurfaceID surface, int texture)
{
    VaGlxSurface glx_surface;
    auto it = glx_surfaces_.find(texture);
    if(it == glx_surfaces_.end()){
        int a = XInitThreads();
        glx_surface = VaapiHelper::create_glx_surface(display,GL_TEXTURE_2D,texture);
        glx_surfaces_[texture] = glx_surface;
    }

    if(glx_surface.gl_surface == nullptr)
        return -1;

    derive_image(display,surface);

    VaapiHelper::glx_copy_surface(surface,glx_surface);

    return 0;

}

int32_t HardwareFrameMapper::map_vaapi_frame_to_x11_texture(VADisplay display, VASurfaceID surface, int texture,int width,int height)
{
    glx_display_ = (Display*)glXGetCurrentDisplay();

    if(glx_fbc_ == 0){

        create_context(kDisplayTypeX11);

        int xscr = DefaultScreen(glx_display_);
        const char *glxext = glXQueryExtensionsString((::Display*)glx_display_, xscr);
        if (!glxext || !strstr(glxext, "GLX_EXT_texture_from_pixmap"))
            return 0;

        int attribs[] = {
            GLX_RENDER_TYPE, GLX_RGBA_BIT, //xbmc
            GLX_X_RENDERABLE, True, //xbmc
            GLX_BIND_TO_TEXTURE_RGBA_EXT, True,
            GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT,
            GLX_BIND_TO_TEXTURE_TARGETS_EXT, GLX_TEXTURE_2D_BIT_EXT,
            GLX_Y_INVERTED_EXT, True,
            GLX_DOUBLEBUFFER, False,
            GLX_RED_SIZE, 8,
            GLX_GREEN_SIZE, 8,
            GLX_BLUE_SIZE, 8,
            GLX_ALPHA_SIZE, 8, //0 for 24 bpp(vdpau)? mpv is 0
            None
        };
        int fbcount;
        GLXFBConfig *fbcs = glXChooseFBConfig((::Display*)glx_display_, xscr, attribs, &fbcount);
        if (!fbcount) {
            MP_LOG_DEAULT("No texture-from-pixmap support");
            return 0;
        }
        if (fbcount)
            glx_fbc_ = fbcs[0];
        XFree(fbcs);
    }


    if (!x11_pixelmap_) {

        XWindowAttributes xwa;
        XGetWindowAttributes(glx_display_, DefaultRootWindow(glx_display_), &xwa);
        x11_pixelmap_ = XCreatePixmap(glx_display_, DefaultRootWindow(glx_display_), width, height, xwa.depth);
        // mpv always use 24 bpp
        if (!x11_pixelmap_) {
            return 0;
        }

        const int depth = xwa.depth;
        if (depth <= 0)
            return false;
        const int attribs[] = {
            GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
            GLX_TEXTURE_FORMAT_EXT, depth == 32 ? GLX_TEXTURE_FORMAT_RGBA_EXT : GLX_TEXTURE_FORMAT_RGB_EXT,
            GLX_MIPMAP_TEXTURE_EXT, False,
            None,
        };

        glx_pixmap_ = glXCreatePixmap(glx_display_, glx_fbc_, x11_pixelmap_, attribs);
    }

    VAStatus status = vaSyncSurface(display, surface);
    if(status != VA_STATUS_SUCCESS)
        return -status;
    // FIXME: invalid surface at the first time vaPutSurface is called. If return false, vaPutSurface will always fail, why?
    status = vaPutSurface(display, surface, x11_pixelmap_
                                , 0, 0, width, height
                                , 0, 0, width, height
                                , NULL, 0, VA_FRAME_PICTURE | VA_SRC_BT709);
    if(status != VA_STATUS_SUCCESS)
        return -status;

    XSync((::Display*)glx_display_, False);
    glActiveTexture( GL_TEXTURE1);
    CHECK_GL_ERROR("glActiveTexture");

    glBindTexture(GL_TEXTURE_2D, texture);
    CHECK_GL_ERROR("glBindTexture");

    glXBindTexImageEXT(glx_display_, glx_pixmap_, GLX_FRONT_EXT, NULL);
    CHECK_GL_ERROR("glXBindTexImageEXT");

    glBindTexture(GL_TEXTURE_2D, 0);
    CHECK_GL_ERROR("glBindTexture");
    return 0;
}
#endif //HAS_VAAPI_GLX
#endif //HAS_VAAPI
