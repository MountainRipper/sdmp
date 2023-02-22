#include <filesystem>
#include <glad/gl.h>
#include <libavutil/frame.h>
#include <logger.h>
#include <imgui.h>
#include "player_example.h"

template<typename T>
inline T max_align(T size){
    uint8_t div_max = 128;
    while(size % div_max) div_max >>= 1;
    return div_max;
}

static const std::string VS_VIDEO = R"(
#version 430
precision mediump float;
  out vec2 v_uv;
  void main() {
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    v_uv.x = (x+1.0)*0.5;
    v_uv.y = (y+1.0)*0.5;
    gl_Position = vec4(x, -y, 0, 1);
})";


static const std::string FS_VIDEO=  R"(
#version 430
in vec2 v_uv;
layout (location = 0) out vec4 fragcolor;
layout (location = 0) uniform sampler2D texture0;
layout (location = 1) uniform sampler2D texture1;
layout (location = 2) uniform sampler2D texture2;
layout (location = 3) uniform sampler2D texture3;

void main() {
    mediump vec2 coordinate = v_uv;
    float y = texture2D(texture0, coordinate).r;
    float u = texture2D(texture1, coordinate).r - 0.5;
    float v = texture2D(texture2, coordinate).r - 0.5;

    //this is only for yuv420 format test
    fragcolor.rgba = vec4(y + 1.403 * v,
                y - 0.344 * u - 0.714 * v,
                y + 1.770 * u,
                1.0);

    //fragcolor.rgba = vec4(v,v,v,1.0);
}
)";



CacheFrame::CacheFrame(const CacheFrame &other){
    texture_y = other.texture_y;
    texture_u = other.texture_u;
    texture_v = other.texture_v;
    frame = other.frame;
}

void CacheFrame::create_to_texture(){
    return;
    AVFrame* av_frame = frame->frame;

    MP_TIMER_NEW(upload_texture);


    glGenTextures(1,&texture_y);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_y);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 GL_RED,
                 av_frame->width, av_frame->height, 0,
                 GL_RED, GL_UNSIGNED_BYTE,
                 av_frame->data[0]);
    //MP_INFO("shared context upload yuv420 1 texture use {}ms",MP_TIMER_MS_RESET(upload_texture));

    glGenTextures(1,&texture_u);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_u);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 GL_RED,
                 av_frame->width/2, av_frame->height/2, 0,
                 GL_RED, GL_UNSIGNED_BYTE,
                 av_frame->data[1]);
    //MP_INFO("shared context upload yuv420 2 texture use {}ms",MP_TIMER_MS_RESET(upload_texture));

    glGenTextures(1,&texture_v);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture_v);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 GL_RED,
                 av_frame->width/2, av_frame->height/2, 0,
                 GL_RED, GL_UNSIGNED_BYTE,
                 av_frame->data[2]);

    MP_INFO("shared context upload yuv420 3 texture use {}ms , pts:{} tex:{},{},{}",MP_TIMER_MS_RESET(upload_texture),av_frame->pts,texture_y,texture_u,texture_v);
}

void CacheFrame::release_texture(){
    if(texture_y){
        glDeleteTextures(1,&texture_y);
        texture_y = 0;
    }
    if(texture_u){
        glDeleteTextures(1,&texture_u);
        texture_u = 0;
    }
    if(texture_v){
        glDeleteTextures(1,&texture_v);
        texture_v = 0;
    }
}


class PlayerEvent : public   mr::sdmp::PlayerEvent{
public:


    PlayerEvent(){

    }
    // PlayerEvent interface
public:
    virtual int32_t on_position(sdmp::Player *player, int64_t ms) override{
        return 0;
    }
    virtual int32_t on_video_frame(sdmp::Player *player, sdmp::FramePointer frame) override{

        std::lock_guard<std::mutex> lock(cache_mutex_);
        totle_frame_count_++;
        CacheFrame cache_frame;
        cache_frame.frame = frame;
        //glfwMakeContextCurrent(win);
        cache_frame.create_to_texture();
        cached_frames_.push(cache_frame);
        //MP_INFO("get video frame type:{}",frame->frame->format);
        return 0;
    }
    CacheFrame pop_frame(){
        CacheFrame frame;
        std::lock_guard<std::mutex> lock(cache_mutex_);
        if(!cached_frames_.empty()){
            frame = cached_frames_.front();
            cached_frames_.pop();
        }
        return frame;
    }
    int32_t cache_frame_count(){
        std::lock_guard<std::mutex> lock(cache_mutex_);
        return cached_frames_.size();
    }
    int32_t totle_frame_count(){
        return totle_frame_count_;
    }
private:
    std::mutex cache_mutex_;
    std::queue<CacheFrame> cached_frames_;
    int32_t totle_frame_count_ = 0;
};


PlayerExample::PlayerExample()
{

}

int32_t PlayerExample::on_video_frame(sdmp::Player *player, sdmp::FramePointer frame){

    std::lock_guard<std::mutex> lock(cache_mutex_);
    totle_frame_count_++;
    CacheFrame cache_frame;
    cache_frame.frame = frame;
    //glfwMakeContextCurrent(win);
    cache_frame.create_to_texture();
    cached_frames_.push(cache_frame);
    //MP_INFO("get video frame type:{}",frame->frame->format);
    return 0;
}

CacheFrame PlayerExample::pop_frame(){
    CacheFrame frame;
    std::lock_guard<std::mutex> lock(cache_mutex_);
    if(!cached_frames_.empty()){
        frame = cached_frames_.front();
        cached_frames_.pop();
    }
    return frame;
}

int32_t PlayerExample::cache_frame_count(){
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return cached_frames_.size();
}

int32_t PlayerExample::totle_frame_count(){
    return totle_frame_count_;
}


int32_t PlayerExample::on_init(void* window)
{
    std::filesystem::path script_path = std::filesystem::path(CMAKE_FILE_DIR) / ".." / "script";
    std::filesystem::path easy_path = std::filesystem::path(CMAKE_FILE_DIR)/"../easy/script-easy";
    g_player = new sdmp::Player(script_path,easy_path);
    g_player->set_event(static_cast<sdmp::PlayerEvent*>(this));


    texture0_ = create_texture(128,128,0);
    texture1_ = create_texture(128,128,128);
    texture2_ = create_texture(128,128,128);
    program_ = create_program(VS_VIDEO.c_str(),FS_VIDEO.c_str());

    glGenVertexArrays(1, &g_vao);
    return 0;
}

int32_t PlayerExample::on_deinit()
{
    return 0;
}

int32_t PlayerExample::on_frame()
{
    //now render the video frame use opengl
    auto frame = pop_frame();

    if(frame.frame){

        MP_TIMER_NEW(upload_texture);
        AVFrame* av_frame = frame.frame->frame;
        GLuint  tex_loc = 0;
        tex_loc = glGetUniformLocation(program_, "texture0");
        glUniform1i(tex_loc, 0);
        glActiveTexture(GL_TEXTURE0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, max_align(av_frame->linesize[0]));
        glBindTexture(GL_TEXTURE_2D, texture0_);
        glTexImage2D(GL_TEXTURE_2D, 0,
                   GL_RED,
                   av_frame->width, av_frame->height, 0,
                   GL_RED, GL_UNSIGNED_BYTE,
                   av_frame->data[0]);
        //MP_INFO("upload yuv420 1 texture use {}ms",MP_TIMER_MS_RESET(upload_texture));

        tex_loc = glGetUniformLocation(program_, "texture1");
        glUniform1i(tex_loc, 1);
        glActiveTexture(GL_TEXTURE1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, max_align(av_frame->linesize[1]));
        glBindTexture(GL_TEXTURE_2D, texture1_);
        glTexImage2D(GL_TEXTURE_2D, 0,
                   GL_RED,
                   av_frame->width/2, av_frame->height/2, 0,
                   GL_RED, GL_UNSIGNED_BYTE,
                   av_frame->data[1]);
        //MP_INFO("upload yuv420 2 texture use {}ms",MP_TIMER_MS_RESET(upload_texture));

        tex_loc = glGetUniformLocation(program_, "texture2");
        glUniform1i(tex_loc, 2);
        glActiveTexture(GL_TEXTURE2);
        glPixelStorei(GL_UNPACK_ALIGNMENT, max_align(av_frame->linesize[2]));
        glBindTexture(GL_TEXTURE_2D, texture2_);
        glTexImage2D(GL_TEXTURE_2D, 0,
                   GL_RED,
                   av_frame->width/2, av_frame->height/2, 0,
                   GL_RED, GL_UNSIGNED_BYTE,
                   av_frame->data[2]);

        //MP_INFO("upload yuv420 3 texture use {}ms , pts:{}",MP_TIMER_MS_RESET(upload_texture),av_frame->pts);
    }
    else{
        GLuint  tex_loc = 0;
        tex_loc = glGetUniformLocation(program_, "texture0");
        glUniform1i(tex_loc, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture0_);

        tex_loc = glGetUniformLocation(program_, "texture1");
        glUniform1i(tex_loc, 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture1_);

        tex_loc = glGetUniformLocation(program_, "texture2");
        glUniform1i(tex_loc, 2);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, texture2_);
    }

    glUseProgram(program_);
    glBindVertexArray(g_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    if(cache_frame_count() > 5){
        static int32_t  drop_frame_count  = 0;
        do{
            frame = pop_frame();
            if(frame.frame){
                drop_frame_count++;
            }
        }while (frame.frame);
        MP_INFO("Drop Video Frames Totle:{} Droped:{}",totle_frame_count() ,drop_frame_count);
    }
    return 0;
}

void PlayerExample::button_callback( int bt, int action, int mods)
{
}

void PlayerExample::cursor_callback(double x, double y)
{
}

void PlayerExample::key_callback(int key, int scancode, int action, int mods)
{
}

void PlayerExample::char_callback( unsigned int key)
{
}

void PlayerExample::error_callback(int err, const char *desc)
{
}

void PlayerExample::resize_callback(int width, int height)
{
}

void PlayerExample::scroll_callback(double xoffset, double yoffset)
{
}

void PlayerExample::command(std::string command)
{
    if(command == "play"){
        g_player->resume();
    }
    else if(command == "pause"){
        g_player->pause();
    }
}
