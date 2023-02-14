#ifndef VAAPIHELPER_H
#define VAAPIHELPER_H

#if defined (HAS_VAAPI)
#include <glad/gl.h>
#include <va/va.h>
struct VaGlxSurface{
    void*   va_display = nullptr;
    int     va_surface = 0;
    int     gl_texture = 0;
    void*   gl_surface = nullptr;
};

class VaapiHelper
{
public:
    VaapiHelper();
#if defined (HAS_VAAPI_GLX)
    static VaGlxSurface create_glx_surface(VADisplay dpy,
                                    GLenum    target,
                                    GLuint    texture);

    static int32_t destroy_glx_surface(VaGlxSurface* surface);
    static int32_t glx_copy_surface(VASurfaceID va_surface,VaGlxSurface& glx_surface);
#endif

};

#endif
#endif // VAAPIHELPER_H
