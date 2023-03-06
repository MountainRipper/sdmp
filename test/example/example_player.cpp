#include <spdlog/fmt/chrono.h>
#include <filesystem>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <libavutil/frame.h>
#include <logger.h>
#include <imgui.h>
#include <ttf/IconsFontAwesome6.h>
#include <tio/tio_hardware_graphic.h>

#include "example_player.h"
#include "imgui_helper.h"

using namespace mr::tio;
static const std::string VS_VIDEO = R"(
  precision mediump float;
  out vec2 v_texCoord;
  void main() {
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    v_texCoord.x = (x+1.0)*0.5;
    v_texCoord.y = (y+1.0)*0.5;
    gl_Position = vec4(x, -y, 0, 1);
})";


CacheFrame::CacheFrame(const CacheFrame &other){
    texture_y = other.texture_y;
    texture_u = other.texture_u;
    texture_v = other.texture_v;
    frame = other.frame;
}

void CacheFrame::create_to_texture(){
    return;
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

int32_t PlayerExample::on_end()
{
    status_string_ = "Finished";
    status_string_history_.push_back(status_string_);
    return 0;
}

int32_t PlayerExample::on_reload()
{
    select_track_ = g_player->track_current();
    auto_replay_ = g_player->auto_replay();
    status_string_history_.push_back(status_string_);
    return 0;
}

int32_t PlayerExample::on_playing()
{
    status_string_ = "Playing";
    status_string_history_.push_back(status_string_);
    return 0;
}

int32_t PlayerExample::on_replaying()
{
    status_string_ = "Replaying";
    status_string_history_.push_back(status_string_);
    return 0;
}

int32_t PlayerExample::on_paused()
{
    status_string_ = "Paused";
    status_string_history_.push_back(status_string_);
    return 0;
}

int32_t PlayerExample::on_resumed()
{
    status_string_ = "Resume";
    status_string_history_.push_back(status_string_);
    return 0;
}

int32_t PlayerExample::on_stoped()
{
    status_string_ = "Stoped";
    status_string_history_.push_back(status_string_);
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


    textures_[0] = create_texture(128,128,0);
    textures_[1] = create_texture(128,128,128);
    textures_[2] = create_texture(128,128,128);

    std::string vertex_shader = VS_VIDEO;
    auto version = (const char*)glGetString(GL_VERSION);
    if(version && strstr(version,"OpenGL ES"))
        vertex_shader = std::string("#version 300 es") + vertex_shader;
    else
        vertex_shader = std::string("#version 430") + vertex_shader;

    auto fragment_shader = TextureIO::reference_shader_software(kGraphicApiOpenGL,kSoftwareFrameI420);

    MP_INFO("{}",fragment_shader)

    program_ = create_program(vertex_shader.c_str(),fragment_shader.c_str());

    texture_locations_[0] = glGetUniformLocation(program_, "tex1");
    texture_locations_[1] = glGetUniformLocation(program_, "tex2");
    texture_locations_[2] = glGetUniformLocation(program_, "tex3");
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
        SoftwareFrame frame = {kSoftwareFrameI420,(uint32_t)av_frame->width,(uint32_t)av_frame->height};
        GraphicTexture textures = {kGraphicApiOpenGL};
        for(int index = 0; index < 4; index++){
            frame.data[index] = av_frame->data[index];
            frame.line_size[index] = av_frame->linesize[index];

            textures.context[index] = textures_[index];
            textures.flags[index] = GL_TEXTURE0+texture_unit_base_+index;
        }
        TextureIO::software_frame_to_graphic(frame,textures);

        MP_INFO("upload yuv420 3 texture use {}ms , pts:{}",MP_TIMER_MS_RESET(upload_texture),av_frame->pts);
    }
    else{

    }

    glUniform1i(texture_locations_[0], texture_unit_base_);
    glUniform1i(texture_locations_[1], texture_unit_base_+1);
    glUniform1i(texture_locations_[2], texture_unit_base_+2);

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

    render_ui();
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
    resized_ = true;
}

void PlayerExample::scroll_callback(double xoffset, double yoffset)
{
}

void PlayerExample::command(std::string command)
{

}

void PlayerExample::render_ui()
{
    if(duration_ == 0)
        duration_ = g_player->duration();

    auto& font_helper = ImGuiHelper::get();

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;


    {
        window_flags |= ImGuiWindowFlags_NoMove;
        if(resized_){
            resized_ = false;
            auto pos = ImGui::GetMainViewport()->GetCenter();
            pos.y *= 1.8;
            ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        }
        ImGui::SetNextWindowBgAlpha(0.35f);
        ImGui::Begin("Control Panel",NULL,window_flags);

        if(ImGui::Button(ICON_FA_PLAY)){
            g_player->resume();
        }
        ImGui::SameLine();
        if(ImGui::Button(ICON_FA_PAUSE)){
            g_player->pause();
        }

        position_ = g_player->position() * 1.0 /  g_player->duration();
        ImGui::SameLine();
        ImGui::SliderFloat(" ", &position_, 0.0, 1.0);
        if (ImGui::IsItemEdited()){
            seek_position_ = position_;
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            MP_INFO("seek_position:{}",seek_position_);
            if(seek_position_ >= 0)
                g_player->play(seek_position_*g_player->duration());
            seek_position_ = -1;
        }

        ImGui::SameLine();
        const std::chrono::duration<float, std::milli> ms_p((int)(position_*duration_));
        const std::chrono::duration<float, std::milli> ms_d((int)duration_);
        std::string  tm_str = fmt::format((ms_d >= std::chrono::hours(1))?"{:%H:%M:%S}/{:%H:%M:%S}":"{:%M:%S}/{:%M:%S}",
                                          ms_p,
                                          ms_d);
        ImGui::Text("%s", tm_str.c_str());

        bool old_autoreplay = auto_replay_;
        ImGui::Checkbox("AutoReplay",&auto_replay_);ImGui::SameLine();
        if(old_autoreplay != auto_replay_){
            g_player->set_auto_replay(auto_replay_);
        }

        int old_channel_mode = channel_mode_;
        ImGui::Text("ChannelMode:"); ImGui::SameLine();
        ImGui::RadioButton("L/R", &channel_mode_, 0); ImGui::SameLine();
        ImGui::RadioButton("L", &channel_mode_, 1); ImGui::SameLine();
        ImGui::RadioButton("R", &channel_mode_, 2);
        if(old_channel_mode != channel_mode_){
            MP_INFO("set channel mode:{}",channel_mode_);
            g_player->set_channel_mode(channel_mode_);
        }
        int32_t tracks = g_player->tracks_count();
        ImGui::SameLine();
        ImGui::Text("\tTrack:");
        int old_track = select_track_;
        for(int index = 0; index < tracks; index++){
            ImGui::SameLine();
            ImGui::RadioButton(fmt::format("T{}",index).c_str(), &select_track_, index);
        }
        if(old_track != select_track_){
            MP_INFO("select track :{}",select_track_);
            g_player->set_track(select_track_);
        }

        std::string format_str = fmt::format("Video:{}x{}@{:.2f}FPS\tAudio:{}Hz {}Ch {}bits Tracks:{}/{}",
                                             g_player->width(),
                                             g_player->height(),
                                             g_player->framerate(),
                                             g_player->samplerate(),
                                             g_player->channels(),
                                             g_player->audio_bits(),
                                             g_player->track_current()+1,
                                             g_player->tracks_count());
        ImGui::Text("%s", format_str.c_str());
        ImGui::End();
    }

    {
        font_helper.use_font_scale(0.75);

        ImGui::SetNextWindowBgAlpha(0.15f);
        ImGui::Begin("Log Panel",NULL,window_flags);
        if(status_string_history_.size() > 20)
            status_string_history_.erase(status_string_history_.begin());

        ImGuiTextBuffer log;
        for(auto& item : status_string_history_){
            log.appendf("%s\n",item.c_str());
        }
        ImGui::TextUnformatted(log.begin(), log.end());
        ImGui::End();

        font_helper.restore_font_size();
    }
}
