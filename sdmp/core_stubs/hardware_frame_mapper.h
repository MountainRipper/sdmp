#ifndef VAAPIFRAMEMAPPER_H
#define VAAPIFRAMEMAPPER_H
#include <inttypes.h>
#include <map>
#include "glad/glad.h"

#if defined(__linux__)
#include "glad/glad_egl.h"
#include "glad/glad_glx.h"
#endif

#if defined (HAS_VAAPI)
#include <va/va.h>
#include "X11/Xlib.h"
#include "vaapi_helper.h"
#endif

extern "C"{
struct AVFrame;
}
typedef unsigned long XID;
typedef void *EGLImage;

#define kErrorFallbackToSoftwareFrame  -100

namespace sdp {


enum DisplayType{
    kDisplayTypeAuto,
    kDisplayTypeDRM,
    kDisplayTypeEGL,
    kDisplayTypeGLX,
    kDisplayTypeX11,
    kDisplayTypeWayland,
    kDisplayTypeWGL,
    kDisplayTypeAGL,
    kDisplayTypeEAGL,
};

class HardwareFrameMapper
{
public:
    HardwareFrameMapper();
    ~HardwareFrameMapper();

    int32_t create_context(DisplayType display_type);

    int32_t map_hardware_frame_to_texture(const AVFrame *frame, int texture);

private:
    int32_t map_drm_frame_to_egl_texture(const AVFrame *frame,int texture);
#if defined (HAS_VAAPI)
    int32_t derive_image(VADisplay display,VASurfaceID surface);
    int32_t map_vaapi_frame_to_egl_texture(VADisplay display,VASurfaceID surface, int texture);
#if defined (HAS_VAAPI_GLX)
    int32_t map_vaapi_frame_to_glx_texture(VADisplay display,VASurfaceID surface, int texture);
    int32_t map_vaapi_frame_to_x11_texture(VADisplay display, VASurfaceID surface, int texture, int width, int height);
#endif//HAS_VAAPI_GLX
#endif//HAS_VAAPI

private:
    int major_version_ = 0;
    int minor_version_ = 0;
    DisplayType     display_type_;
    EGLImage        egl_image_      = nullptr;

#if defined (HAS_VAAPI)
    VADisplay       display_handle_;
    VAImage         va_image        = {0};
    VABufferInfo    va_buffer_info  = {0};
    uintptr_t       native_display_handle_ = 0;
#if defined (HAS_VAAPI_GLX)
    Display         *glx_display_   = nullptr;
    GLXFBConfig     glx_fbc_        = nullptr;
    GLXPixmap       glx_pixmap_     = 0;
    Pixmap          x11_pixelmap_   = 0;
    std::map<int,VaGlxSurface>  glx_surfaces_;
#endif//HAS_VAAPI_GLX
#endif//HAS_VAAPI

};

}

#endif // VAAPIFRAMEMAPPER_H
