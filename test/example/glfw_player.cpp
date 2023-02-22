#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/gl.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <libavutil/pixfmt.h>
#include <logger.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <imgui.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include "player_example.h"
#include "IconsFontAwesome6.h"
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

#if defined(__linux)
#include <unistd.h>
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

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

  GLFWwindow *window = NULL;
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
  const char* glsl_version = "#version 100";
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
  use_gles = true;
#else
  const char* glsl_version = "#version 150";
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

  window = glfwCreateWindow(win_w, win_h, "SDMP Usecase Test", NULL, NULL);
  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwSetFramebufferSizeCallback(window, resize_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetCharCallback(window, char_callback);
  glfwSetCursorPosCallback(window, cursor_callback);
  glfwSetMouseButtonCallback(window, button_callback);
  glfwSetScrollCallback(window, scroll_callback);

#if USE_GL
  glfwMakeContextCurrent(window);
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
  native_window = (void *)glfwGetX11Window(window);
  native_window_glx = (void *)glfwGetGLXWindow(window);
  native_display = (void *)glfwGetX11Display();
  opengl_context = (void *)glfwGetGLXContext(window);
#endif
#if USE_GL_WIN
  native_window = (void *)glfwGetWin32Window(win);
  opengl_context = (void *)glfwGetWGLContext(win);
#endif

#endif

  glfwMakeContextCurrent(window);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  //ImGui::StyleColorsLight();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);
  ImGuiIO& io = ImGui::GetIO();
  float baseFontSize = 24.0f; // 13.0f is the size of the default font. Change to the font size you use.
  ImFont* font = io.Fonts->AddFontFromFileTTF("NotoSansSC-Regular.otf",baseFontSize,NULL,io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
  float iconFontSize = baseFontSize * 2.0f / 3; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly

  // merge in icons from Font Awesome
  static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
  ImFontConfig icons_config;
  icons_config.MergeMode = true;
  icons_config.PixelSnapH = true;
  icons_config.GlyphMinAdvanceX = iconFontSize;
  io.Fonts->AddFontFromFileTTF( FONT_ICON_FILE_NAME_FAS, iconFontSize, &icons_config, icons_ranges );
  bool show_demo_window = true;
  io.Fonts->Build();

  /* -------------------------------------------- */

  g_example = new PlayerExample();
  g_example->on_init(window);

  while (!glfwWindowShouldClose(window)) {

    // glfwMakeContextCurrent(win);
    // glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, win_w, win_h);
    // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    g_example->on_frame();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    {
        ImGui::Begin("Hello, world!");
        //ImGui::PushFont(font_zhch);
        ImGui::Button( ICON_FA_CALENDAR" 开播设置");
        ImGui::SameLine();
        ImGui::Checkbox("Demo Window", &show_demo_window);

        if(ImGui::Button(ICON_FA_PLAY)){
            g_example->command("play");
        }
        ImGui::SameLine();
        if(ImGui::Button(ICON_FA_PAUSE)){
            g_example->command("pause");
        }
        ImGui::SameLine();
        ImGui::Button(ICON_FA_STOP);
        ImGui::SameLine();
        //ImGui::PopFont();
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#if defined(__linux)
    //  usleep(16e3);
#endif

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  g_example->on_deinit();
  delete g_example;

  glfwTerminate();

  return EXIT_SUCCESS;
}
