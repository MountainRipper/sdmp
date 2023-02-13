#include "vaapi_helper.h"

#if defined (HAS_VAAPI)

#if defined (HAS_VAAPI_GLX)
#include <va/va_glx.h>
#include <va/va_x11.h>
#endif



VaapiHelper::VaapiHelper()
{

}


#if defined (HAS_VAAPI_GLX)
VaGlxSurface VaapiHelper::create_glx_surface(VADisplay dpy, GLenum target, GLuint texture)
{

    uintptr_t native_display_handle_ = (uintptr_t)XOpenDisplay(NULL);
    int ret = XInitThreads();
    if(ret == 0){
        return VaGlxSurface();
    }
    auto display_handle_ =  vaGetDisplayGLX((Display*)native_display_handle_);

    int major_version_ = 0;
    int minor_version_ = 0;
    auto statusa  = vaInitialize(display_handle_,&major_version_,&minor_version_);

    VaGlxSurface surface;
    surface.va_display = dpy;
    surface.gl_texture = texture;

    VAStatus status = vaCreateSurfaceGLX(dpy,target,texture,&surface.gl_surface);
    if(status != VA_STATUS_SUCCESS)
        return VaGlxSurface();
    return surface;
}

int32_t VaapiHelper::destroy_glx_surface(VaGlxSurface *surface)
{
    if(surface->va_display == nullptr || surface->gl_surface == nullptr)
        return -1;
    auto stasut = vaDestroySurfaceGLX((VADisplay)surface->va_display,surface->gl_surface);
    return (stasut==VA_STATUS_SUCCESS)?0:-1;
}

int32_t VaapiHelper::glx_copy_surface(VASurfaceID va_surface,VaGlxSurface& glx_surface)
{
    VAStatus status = vaCopySurfaceGLX(glx_surface.va_display,glx_surface.gl_surface,va_surface,0);
    if(status != VA_STATUS_SUCCESS)
        return -status;

    status = vaSyncSurface(glx_surface.va_display, va_surface);
    if(status != VA_STATUS_SUCCESS)
        return -status;
    return status;
}
#endif//HAS_VAAPI_GLX

#endif//HAS_VAAPI
