#define USE_GL 1

/* -------------------------------------------- */

#if defined(__linux)
#  define USE_GL_LINUX USE_GL
#  define GLFW_EXPOSE_NATIVE_X11
#  define GLFW_EXPOSE_NATIVE_GLX
#  define GLFW_EXPOSE_NATIVE_EGL
#endif

#if defined(_WIN32)
#  define USE_GL_WIN USE_GL
#  define GLFW_EXPOSE_NATIVE_WGL
#  define GLFW_EXPOSE_NATIVE_WIN32
#endif

/* -------------------------------------------- */

#if defined(__linux)
#  include <unistd.h>
#endif

/* -------------------------------------------- */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <thread>
#include <glad/gl.h>
//#include <glad/gles2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <sdpi_factory.h>
/* -------------------------------------------- */

void button_callback(GLFWwindow* win, int bt, int action, int mods);
void cursor_callback(GLFWwindow* win, double x, double y);
void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods);
void char_callback(GLFWwindow* win, unsigned int key);
void error_callback(int err, const char* desc);
void resize_callback(GLFWwindow* window, int width, int height);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);


void init_callback(GLFWwindow* window);
void frame_callback(GLFWwindow* window);
void deinit_callback(GLFWwindow* window);
static int print_shader_compile_info(uint32_t shader);

#if defined(USE_FILAMENT)
extern GFXBase* create_filament_tester();
#endif
#if defined(USE_BGFX)
extern GFXBase* create_bgfx_tester();
#endif
/* -------------------------------------------- */

uint32_t win_w = 1280;
uint32_t win_h = 720;

/* -------------------------------------------- */

static const std::string VS = R"(#version 430
precision mediump float;
  out vec2 v_uv;
  void main() {
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    v_uv.x = (x+1.0)*0.5;
    v_uv.y = (y+1.0)*0.5;
    gl_Position = vec4(x, y, 0, 1);
  }
)";

static const std::string FS = R"(#version 430
  layout (location = 0) uniform sampler2D u_tex;
  layout (location = 0) out vec4 fragcolor;
  in vec2 v_uv;

  void main() {
    fragcolor = vec4(1.0, 0.14, 0.0, 1.0);
    fragcolor.rgba = texture(u_tex, v_uv).rgba;
    //fragcolor.a = v_uv.x;
  }
)";
static const std::string VSES = R"(#version 320 es
  out vec2 v_uv;
  void main() {
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    v_uv.x = (x+1.0)*0.5;
    v_uv.y = (y+1.0)*0.5;
    gl_Position = vec4(x, y, 0, 1);
  }
)";

static const std::string FSES = R"(#version 320 es
precision mediump float;
  layout (location = 0) uniform sampler2D u_tex;
  layout (location = 0) out vec4 fragcolor;
  in vec2 v_uv;

  void main() {
    fragcolor = vec4(1.0, 0.14, 0.0, 1.0);
    fragcolor.rgba = texture(u_tex, v_uv).rgba;
  }
)";

/* -------------------------------------------- */
void logEglError(const char* name) noexcept {
    const char* err;
    switch (eglGetError()) {
        case EGL_NOT_INITIALIZED:       err = "EGL_NOT_INITIALIZED";    break;
        case EGL_BAD_ACCESS:            err = "EGL_BAD_ACCESS";         break;
        case EGL_BAD_ALLOC:             err = "EGL_BAD_ALLOC";          break;
        case EGL_BAD_ATTRIBUTE:         err = "EGL_BAD_ATTRIBUTE";      break;
        case EGL_BAD_CONTEXT:           err = "EGL_BAD_CONTEXT";        break;
        case EGL_BAD_CONFIG:            err = "EGL_BAD_CONFIG";         break;
        case EGL_BAD_CURRENT_SURFACE:   err = "EGL_BAD_CURRENT_SURFACE";break;
        case EGL_BAD_DISPLAY:           err = "EGL_BAD_DISPLAY";        break;
        case EGL_BAD_SURFACE:           err = "EGL_BAD_SURFACE";        break;
        case EGL_BAD_MATCH:             err = "EGL_BAD_MATCH";          break;
        case EGL_BAD_PARAMETER:         err = "EGL_BAD_PARAMETER";      break;
        case EGL_BAD_NATIVE_PIXMAP:     err = "EGL_BAD_NATIVE_PIXMAP";  break;
        case EGL_BAD_NATIVE_WINDOW:     err = "EGL_BAD_NATIVE_WINDOW";  break;
        case EGL_CONTEXT_LOST:          err = "EGL_CONTEXT_LOST";       break;
        default:                        err = "unknown";                break;
    }
    std::cerr << name << " failed with " << err << std::endl;
}

int main(int argc, char* argv[]) {

  glfwSetErrorCallback(error_callback);

  if(!glfwInit()) {
    printf("Error: cannot setup glfw.\n");
    exit(EXIT_FAILURE);
  }
#ifdef USE_EGL
  glfwWindowHint(GLFW_CONTEXT_CREATION_API,GLFW_EGL_CONTEXT_API);
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


  GLFWwindow* win = NULL;

  win = glfwCreateWindow(win_w, win_h, "Filament Shared OpenGL Context", NULL, NULL);
  if(!win) {
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

  fprintf(stderr,"GL_VENDOR : %s\n", glGetString(GL_VENDOR) );
  fprintf(stderr,"GL_VERSION  : %s\n", glGetString(GL_VERSION) );
  fprintf(stderr,"GL_RENDERER : %s\n", glGetString(GL_RENDERER) );
  if(eglGetCurrentContext()){
      auto display = glfwGetEGLDisplay();
      fprintf(stderr,"EGL_VERSION : %s\n", eglQueryString(display,EGL_VERSION) );
      fprintf(stderr,"EGL_VENDOR : %s\n", eglQueryString(display,EGL_VENDOR) );
  }
  if(glXGetCurrentContext()){
      auto display = glXGetCurrentDisplay();
      int majv,minv;
      glXQueryVersion(display,&majv,&minv);
      fprintf(stderr,"GLX VERSION : %d.%d\n", majv,minv);
  }

  //fprintf(stderr,"GL_EXTENSIONS : %s\n", glGetString(GL_EXTENSIONS) );
#endif

  /* -------------------------------------------- */
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_DITHER);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  /* -------------------------------------------- */
  void* native_display = nullptr;
  void* native_window = nullptr;
  void* opengl_context = nullptr;
  void* native_window_glx = nullptr;

  EGLDisplay mEGLDisplay = nullptr;
#if USE_GL_LINUX

#ifdef USE_EGL
  native_window = (void*)glfwGetX11Window(win);
  native_display = (void*)glfwGetEGLDisplay();
  opengl_context = (void*)glfwGetEGLContext(win);
#else
  native_window = (void*)glfwGetX11Window(win);
  native_window_glx = (void*)glfwGetGLXWindow(win);
  native_display = (void*)glfwGetX11Display();
  opengl_context = (void*)glfwGetGLXContext(win);
#endif
#if USE_GL_WIN
  native_window = (void*)glfwGetWin32Window(win);
  opengl_context = (void*)glfwGetWGLContext(win);
#endif


#ifdef USE_GLES
  setenv("EGL_HOST_OPENGL_API","ES3",1);
#endif

#ifdef USE_EGL
  char string[32];
  sprintf(string,"%ld",(intptr_t)native_display);
  setenv("EGL_HOST_DISPLAY",string,1);
  setenv("BGFX_RUNTIME_REQUEST_EGL","ON",1);
  const char* disp_s = getenv("EGL_HOST_DISPLAY");
  if(disp_s != nullptr)
      mEGLDisplay = (EGLDisplay)(intptr_t)atol(disp_s);
  else
      mEGLDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
#endif

#endif


  GLuint target_texture = 0;
  glGenTextures(1,&target_texture);
  glBindTexture(GL_TEXTURE_2D, target_texture);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  uint8_t* image = new uint8_t[win_w* win_h*4];
  memset(image,255,win_w* win_h*4);
  glTexImage2D(GL_TEXTURE_2D, 0,
             GL_RGBA,
             win_w, win_h, 0,
             GL_RGBA, GL_UNSIGNED_BYTE,
             image);

  /*
     Create the shader and the necessary GL objects that we use
     to render the result of what Filament renders into the
     framebuffer.
   */
  uint32_t vao = 0;
  uint32_t vert = 0;
  uint32_t frag = 0;
  uint32_t prog = 0;
#ifdef USE_GLES
  const char* vss = VSES.c_str();
  const char* fss = FSES.c_str();
#else
  const char* vss = VS.c_str();
  const char* fss = FS.c_str();
#endif
  glfwMakeContextCurrent(win);

  vert = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vert, 1, &vss, nullptr);
  glCompileShader(vert);
  print_shader_compile_info(vert);

  frag = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(frag, 1, &fss, nullptr);
  glCompileShader(frag);
  print_shader_compile_info(frag);

  prog = glCreateProgram();
  glAttachShader(prog, vert);
  glAttachShader(prog, frag);
  glLinkProgram(prog);

  glGenVertexArrays(1, &vao);
  /* -------------------------------------------- */

  init_callback(win);

  while(!glfwWindowShouldClose(win)) {

    glfwMakeContextCurrent(win);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, win_w, win_h);
    glClearColor(0.0f, 0.6f, 0.13f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    frame_callback(win);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, target_texture);
    glUseProgram(prog);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

#if defined(__linux)
    usleep(16e3);
#endif

    glfwSwapBuffers(win);
    glfwPollEvents();
  }

  deinit_callback(win);

  glfwTerminate();

  return EXIT_SUCCESS;
}

/* -------------------------------------------- */

void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods) {

  if (GLFW_RELEASE == action) {
    return;
  }

  switch(key) {
    case GLFW_KEY_ESCAPE: {
      glfwSetWindowShouldClose(win, GL_TRUE);
      break;
    }
  };
}

void error_callback(int err, const char* desc) {
  printf("GLFW error: %s (%d)\n", desc, err);
}
/* -------------------------------------------- */

/*
   Checks the compile info, if it didn't compile we return < 0,
   otherwise 0
*/
static int print_shader_compile_info(uint32_t shader) {

  GLint status = 0;
  GLint count = 0;
  GLchar* error = NULL;

  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if(status) {
    return 0;
  }

  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &count);
  if (0 == count) {
    return 0;
  }

  error = (GLchar*) malloc(count);
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &count);
  if(count <= 0) {
    free(error);
    error = NULL;
    return 0;
  }

  glGetShaderInfoLog(shader, count, NULL, error);

  fprintf(stderr,"SHADER COMPILE ERROR\n");
  fprintf(stderr,"--------------------------------------------------------\n");
  fprintf(stderr,"%s\n", error);
  fprintf(stderr,"--------------------------------------------------------\n");

  free(error);
  error = NULL;

  return -1;
}

/* -------------------------------------------- */

void resize_callback(GLFWwindow* window, int width, int height) { }
void cursor_callback(GLFWwindow* win, double x, double y) { }
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) { }
void button_callback(GLFWwindow* win, int bt, int action, int mods) { }
void char_callback(GLFWwindow* win, unsigned int key) { }

class PlayerEvent : public sdp::IGraphEvent{
public:
    int32_t on_graph_init(sdp::IGraph *graph) override {
        auto& vm = *graph->vm();
        vm["qsdpContext"] = this;
        vm["qsdpStatusEvent"] = &PlayerEvent::on_status;
        fprintf(stderr,"Graph inited\n");
        return 0;
    }

    int32_t on_graph_created(sdp::IGraph *graph) override {
        graph->cmd_play();
        fprintf(stderr,"Graph created\n");
        return 0;
    }

    int32_t on_graph_error(sdp::IGraph *graph, int32_t error_code) override {
        fprintf(stderr,"Graph errored\n");
        return 0;
    }

    int32_t on_graph_master_loop(sdp::IGraph *graph) override {
        return 0;
    }

    void on_status(sol::table){
        fprintf(stderr,"PlayerEvent on_status\n");
    }

};

PlayerEvent *event = nullptr;
std::shared_ptr<sdp::IGraph> graph;
void init_callback(GLFWwindow* window){
    sdp::Factory::initialize_factory();

    std::filesystem::path script_path = std::filesystem::path(CMAKE_FILE_DIR) / ".." / "script";

    sdp::FeatureMap features;
    sdp::Factory::initialnize_engine(script_path.string(),script_path/"engine.lua",features);

    event  = new PlayerEvent();
    graph = sdp::Factory::create_graph_from(script_path/"player.lua",static_cast<sdp::IGraphEvent*>(event));
}
void frame_callback(GLFWwindow* window){

}
void deinit_callback(GLFWwindow* window){
    graph->cmd_stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    graph = nullptr;
}

/* -------------------------------------------- */
