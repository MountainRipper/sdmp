#include <filesystem>
#include <glad/gl.h>
#include <imgui.h>
#include <mrcommon/imgui_mr.h>
#include <ttf/IconsFontAwesome6.h>
#include <libavutil/frame.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/chrono.h>
#include <sdmpi_factory.h>

#include "example_player.h"

MR_MR_SDL_RUNNER_SHOWCASE(PlayerExample)

using namespace mr::tio;


PlayerExample::PlayerExample()
{
}

int32_t PlayerExample::on_video_frame(sdmp::Player *player, sdmp::FramePointer frame){

    std::lock_guard<std::mutex> lock(cache_mutex_);
    totle_frame_count_++;
    cached_frames_.push(frame);
    //MR_INFO("get video frame type:{}",frame->frame->format);
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

sdmp::FramePointer PlayerExample::pop_frame(){
    sdmp::FramePointer frame;
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

int32_t PlayerExample::on_set_params(cxxopts::Options &options)
{
    return 0;
}

int32_t PlayerExample::on_pre_init(cxxopts::ParseResult &options_result, uint32_t &window_flags)
{
    return 0;
}
int32_t PlayerExample::on_init(void *window,int width, int height)
{
    width_ = width;
    height_ = height;

    std::filesystem::path script_path = std::filesystem::path(CMAKE_FILE_DIR) / ".." / "script";
    std::filesystem::path easy_path = std::filesystem::path(CMAKE_FILE_DIR)/"../easy/script-easy";
    g_player = new sdmp::Player(script_path.string(),easy_path.string());
    g_player->set_event(static_cast<sdmp::PlayerEvent*>(this));

    renderer_ = mr::sdmp::Factory::create_object(CLSID_ATTENDANT_VIDEO_GRAPHIC_RENDERER);
    return 0;
}

int32_t PlayerExample::on_deinit()
{
    renderer_ = nullptr;
    return 0;
}

int32_t PlayerExample::on_frame()
{
    glClearColor(1.0,0.5,0.0,0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    SoftwareFrameFormat render_format = kSoftwareFormatRGBA32;
    //now render the video frame use opengl
    auto cache_frame = pop_frame();

    static int rotate = 0;
    if((++rotate) > 360)
        rotate = 0;
    sdmp::IFilterExtentionVideoRenderer::RenderParam param = {0,0,width_,height_,(float)0,1,1,0,0};
    if(cache_frame){
        renderer_->render_video_frame(cache_frame,param);
    }
    else{
        renderer_->render_current_frame(param);
    }


    if(cache_frame_count() > 5){
        static int32_t  drop_frame_count  = 0;
        do{
            cache_frame = pop_frame();
            if(cache_frame){
                drop_frame_count++;
            }
        }while (cache_frame);
        MR_INFO("Drop Video Frames Totle:{} Droped:{}",totle_frame_count() ,drop_frame_count);
    }

    render_ui();
    return 0;
}

void PlayerExample::button_callback(int bt, int type, int clicks, double x, double y)
{

}


void PlayerExample::cursor_callback(double x, double y)
{
}

void PlayerExample::key_callback(int key, int type, int scancode, int mods)
{

}

void PlayerExample::error_callback(int err, const char *desc)
{
}

void PlayerExample::resize_callback(int width, int height)
{
    width_ = width;
    height_ = height;
    resized_ = true;
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
            MR_INFO("seek_position:{}",seek_position_);
            if(seek_position_ >= 0)
                g_player->play(seek_position_*g_player->duration());
            seek_position_ = -1;
        }

        ImGui::SameLine();
        const std::chrono::duration<float, std::milli> ms_p((int)(position_*duration_));
        const std::chrono::duration<float, std::milli> ms_d((int)duration_);
        std::string  tm_str;
        tm_str = fmt::format((ms_d >= std::chrono::hours(1))?"{:%H:%M:%S}/{:%H:%M:%S}":"{:%M:%S}/{:%M:%S}",
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
            MR_INFO("set channel mode:{}",channel_mode_);
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
            MR_INFO("select track :{}",select_track_);
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
