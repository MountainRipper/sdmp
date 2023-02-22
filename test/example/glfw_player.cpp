#define USE_GL 1

/* -------------------------------------------- */

#if defined(__linux)
#define USE_GL_LINUX USE_GL
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#define GLFW_EXPOSE_NATIVE_EGL
#endif

#if defined(_WIN32)
#define USE_GL_WIN USE_GL
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

/* -------------------------------------------- */

#if defined(__linux)
#include <unistd.h>
#endif

/* -------------------------------------------- */
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <glad/glx.h>
#include <glad/gl.h>
// #include <glad/gles2.h>
#include "player_example.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <libavutil/pixfmt.h>
#include <logger.h>

using namespace mr;

uint32_t win_w = 1280;
uint32_t win_h = 720;
ExampleBase *g_example = nullptr;

void key_callback(GLFWwindow *win, int key, int scancode, int action,
                  int mods) {

  if (GLFW_RELEASE == action) {
    return;
  }

  switch (key) {
  case GLFW_KEY_ESCAPE: {
    glfwSetWindowShouldClose(win, GL_TRUE);
    break;
  }
  };

  g_example->key_callback(key,scancode,action,mods);
}

void resize_callback(GLFWwindow *window, int width, int height) {
  win_w = width;
  win_h = height;
  g_example->resize_callback(width, height);
}
void cursor_callback(GLFWwindow *window, double x, double y) {
  g_example->cursor_callback(x, y);
}
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  g_example->scroll_callback(xoffset, yoffset);
}
void button_callback(GLFWwindow *window, int bt, int action, int mods) {
  g_example->button_callback(bt, action, mods);
}
void char_callback(GLFWwindow *window, unsigned int key) {
  g_example->char_callback(key);
}
void error_callback(int err, const char *desc) {
  printf("GLFW error: %s (%d)\n", desc, err);
}

int main(int argc, char *argv[]) {

  GLFWwindow *win = NULL;
  void *native_display = nullptr;
  void *native_window = nullptr;
  void *opengl_context = nullptr;
  void *native_window_glx = nullptr;

  glfwSetErrorCallback(error_callback);

  if (!glfwInit()) {
    printf("Error: cannot setup glfw.\n");
    exit(EXIT_FAILURE);
  }
#ifdef USE_EGL
  glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
#endif

  bool use_gles = false;
#ifdef USE_GLES
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
  use_gles = true;
#else
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

  win = glfwCreateWindow(win_w, win_h, "SDMP Usecase Test", NULL, NULL);
  if (!win) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwSetFramebufferSizeCallback(win, resize_callback);
  glfwSetKeyCallback(win, key_callback);
  glfwSetCharCallback(win, char_callback);
  glfwSetCursorPosCallback(win, cursor_callback);
  glfwSetMouseButtonCallback(win, button_callback);
  glfwSetScrollCallback(win, scroll_callback);

#if USE_GL
  glfwMakeContextCurrent(win);
  glfwSwapInterval(1);

  if (!gladLoaderLoadGL()) {
    printf("Cannot load GL.\n");
    exit(1);
  }

  //  if (!gladLoaderLoadGLES2()) {
  //    printf("Cannot load GLES2.\n");
  //    //exit(1);
  //  }

  fprintf(stderr, "GL_VENDOR : %s\n", glGetString(GL_VENDOR));
  fprintf(stderr, "GL_VERSION  : %s\n", glGetString(GL_VERSION));
  fprintf(stderr, "GL_RENDERER : %s\n", glGetString(GL_RENDERER));
  if (eglGetCurrentContext()) {
    auto display = glfwGetEGLDisplay();
    fprintf(stderr, "EGL_VERSION : %s\n", eglQueryString(display, EGL_VERSION));
    fprintf(stderr, "EGL_VENDOR : %s\n", eglQueryString(display, EGL_VENDOR));
  }
  if (glXGetCurrentContext()) {
    auto display = glXGetCurrentDisplay();
    int majv, minv;
    glXQueryVersion(display, &majv, &minv);
    fprintf(stderr, "GLX VERSION : %d.%d\n", majv, minv);
  }

  // fprintf(stderr,"GL_EXTENSIONS : %s\n", glGetString(GL_EXTENSIONS) );
#endif

  /* -------------------------------------------- */
  // glEnable(GL_TEXTURE_2D);
  // VGFX_GL_CHECK(mp::Logger::kLogLevelError,"glEnable(GL_TEXTURE_2D)");
  glDisable(GL_DEPTH_TEST);
  VGFX_GL_CHECK(mp::Logger::kLogLevelError, "glDisable(GL_DEPTH_TEST)");
  glDisable(GL_DITHER);
  VGFX_GL_CHECK(mp::Logger::kLogLevelError, "glDisable(GL_DITHER)");
  glEnable(GL_BLEND);
  VGFX_GL_CHECK(mp::Logger::kLogLevelError, "glEnable(GL_BLEND)");
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  VGFX_GL_CHECK(mp::Logger::kLogLevelError,
                "glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)");
  /* -------------------------------------------- */

  EGLDisplay mEGLDisplay = nullptr;
#if USE_GL_LINUX

#ifdef USE_EGL
  native_window = (void *)glfwGetX11Window(win);
  native_display = (void *)glfwGetEGLDisplay();
  opengl_context = (void *)glfwGetEGLContext(win);
#else
  native_window = (void *)glfwGetX11Window(win);
  native_window_glx = (void *)glfwGetGLXWindow(win);
  native_display = (void *)glfwGetX11Display();
  opengl_context = (void *)glfwGetGLXContext(win);
#endif
#if USE_GL_WIN
  native_window = (void *)glfwGetWin32Window(win);
  opengl_context = (void *)glfwGetWGLContext(win);
#endif

#ifdef USE_GLES
  setenv("EGL_HOST_OPENGL_API", "ES3", 1);
#endif

#ifdef USE_EGL
  char string[32];
  sprintf(string, "%ld", (intptr_t)native_display);
  setenv("EGL_HOST_DISPLAY", string, 1);
  setenv("BGFX_RUNTIME_REQUEST_EGL", "ON", 1);
  const char *disp_s = getenv("EGL_HOST_DISPLAY");
  if (disp_s != nullptr)
    mEGLDisplay = (EGLDisplay)(intptr_t)atol(disp_s);
  else
    mEGLDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
#endif

#endif

  glfwMakeContextCurrent(win);
  /* -------------------------------------------- */

  g_example = new PlayerExample();
  g_example->on_init(win);

  while (!glfwWindowShouldClose(win)) {

    // glfwMakeContextCurrent(win);
    // glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, win_w, win_h);
    // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    g_example->on_frame();

#if defined(__linux)
    //  usleep(16e3);
#endif

    glfwSwapBuffers(win);
    glfwPollEvents();
  }

  g_example->on_deinit();
  delete g_example;

  glfwTerminate();

  return EXIT_SUCCESS;
}
