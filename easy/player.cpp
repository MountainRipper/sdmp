#include <filesystem>
#include <sol/sol.hpp>
#include <sdmp/interface/sdmpi_factory.h>
#include <sdmp/interface/sdmpi_filter_extentions.h>
#include <mr/logger.h>
#include "player.h"

namespace mr {
namespace sdmp {

class PlayerPrivateContex : public IFilterExtentionVideoOutputProxy::Observer{
public:
    PlayerPrivateContex(Player* player,const std::string& script)
        :player_(player){

        graph = sdmp::Factory::create_graph_from(script,[this](sdmp::IGraph* graph,GraphEventType event,int32_t param){
            if(event == kGraphEventLoaded){
                sol::state& state = *graph->vm();
                state["luaCallNative"] = &PlayerPrivateContex::lua_call_native;
                state["nativeContext"] = this;
                return 0;
            }
            else if(event == kGraphEventCreated){
                ComPointer<IFilterExtentionVideoOutputProxy> render;
                sdmp::GraphHelper::append_video_observer(graph,"",static_cast<IFilterExtentionVideoOutputProxy::Observer*>(this),true,render);
                graph->execute_command(kGraphCommandConnect);
                graph->execute_command(kGraphCommandPlay);
                return 0;
            }
            return 0;
        });
    }
    // Observer interface
public:
    virtual int32_t proxy_render_frame(std::shared_ptr<Frame> frame) override
    {
        if(event_)
            return event_->on_video_frame(nullptr,frame);
        return 0;
    }
private:
    int32_t lua_call_native(const std::string& type,sol::variadic_args args){
        const sol::table& graph_tb = SDMP_GRAPH_GET_CONTEXT(graph);
        if(type == "connect-done"){
            std::string uri = graph_tb["info"]["uri"];
            double duration = graph_tb["info"]["duration"].get_or<double>(0);            
            duration_   = std::round(duration);
            samplerate_ = graph_tb["info"]["audioSamplerate"].get_or<double>(0);
            channels_   = graph_tb["info"]["audioChannels"].get_or<double>(0);
            audio_bits_ = graph_tb["info"]["audioBits"].get_or<double>(0);
            tracks_     = graph_tb["info"]["tracks"].get_or<double>(0);
            width_  = graph_tb["info"]["videoWidth"].get_or<double>(0);
            height_ = graph_tb["info"]["videoHeight"].get_or<double>(0);
            framerate_ = graph_tb["info"]["videoFps"].get_or<double>(0);
            volume = graph_tb["info"]["volume"].get_or<double>(0);
            track_ = graph_tb["info"]["track"].get_or<double>(0) - 1;

            auto_replay_ = graph_tb["info"]["autoReplay"];
            if(event_)
                event_->on_reload();
        }
        else if(type == "track-changed"){
            track_ = graph_tb["info"]["track"].get_or<double>(0) - 1;
        }
        else if(type == "playing"){
            end_of_stream_ = false;
            if(event_)
                event_->on_playing();
        }
        else if(type == "replaying"){
            end_of_stream_ = false;
            if(event_)
                event_->on_replaying();
        }
        else if(type == "paused"){
            end_of_stream_ = false;
            if(event_)
                event_->on_paused();
        }
        else if(type == "resume"){
            end_of_stream_ = false;
            if(event_)
                event_->on_resumed();
        }
        else if(type == "end-of-stream"){
            end_of_stream_ = true;
            if(event_)
                event_->on_end();
        }
        else if(type == "stoped"){
            end_of_stream_ = false;
            if(event_)
                event_->on_stoped();
        }
        else if(type == "position"){
            position_ = args[0].as<double>();
        }
        else {
            MR_INFO(type);
        }

        return 0;
    }
public:
    Player* player_ = nullptr;
    PlayerEvent* event_ = nullptr;
    std::shared_ptr<sdmp::IGraph> graph;
    bool     auto_replay_ = false;
    int64_t  duration_ = 0;
    int64_t  position_ = 0;
    uint32_t samplerate_ = 0;
    uint8_t  channels_ = 0;
    uint8_t  audio_bits_ = 0;
    uint8_t  tracks_ = 0;
    uint8_t  track_ = 0;
    int16_t  width_ = 0;
    int16_t  height_ = 0;
    float    framerate_ = 0;
    float    volume = 0;
    bool     end_of_stream_ = false;
};

Player::Player(const std::string& base_scipts_dir, const std::string& easy_scipts_dir)
{
    sdmp::Factory::initialize_factory();
    sdmp::FeatureMap features;
    sdmp::Factory::initialnize_engine(base_scipts_dir,std::filesystem::path(base_scipts_dir)/"engine.lua",features);

    context_ = new PlayerPrivateContex(this,std::filesystem::path(easy_scipts_dir)/"player.lua");
}

int32_t Player::set_event(PlayerEvent *event)
{
    context_->event_ = event;
    return 0;
}

int32_t Player::play(int32_t position_ms)
{
    if(position_ms == kPositionCurrent && context_->end_of_stream_)
        position_ms = 0;
    context_->graph->execute_command_async(kGraphCommandSeek,Arguments().add(position_ms));
    context_->graph->execute_command_async(kGraphCommandPlay);
    return 0;
}

int32_t Player::pause()
{
    context_->graph->execute_command_async(kGraphCommandPause);
    return 0;
}

int32_t Player::resume()
{
    if(context_->end_of_stream_)
        return play(0);
    context_->graph->execute_command_async(kGraphCommandPlay);
    return 0;
}

int32_t Player::stop()
{
    context_->graph->execute_command_async(kGraphCommandStop);
    return 0;
}

int32_t Player::set_auto_replay(bool replay)
{
    context_->graph->execute_command_async(kGraphCommandCallLuaFunction,Arguments("nativeSetAutoReplay").add(replay));
    return 0;
}

int32_t Player::position()
{
    return (context_->end_of_stream_)?(context_->duration_):(context_->position_);
}

int32_t Player::set_position(int32_t ms)
{
    return 0;
}

float Player::volume()
{
    return context_->volume;
}

int32_t Player::set_volume(float volume)
{
    context_->graph->execute_command_async(kGraphCommandCallLuaFunction,Arguments("nativeSetVolume").add(volume));
    return 0;
}

int32_t Player::auto_replay()
{
    return context_->auto_replay_;
}

int32_t Player::duration()
{
    return context_->duration_;
}

float Player::framerate()
{
    return context_->framerate_;
}

int32_t Player::width()
{
    return context_->width_;
}

int32_t Player::height()
{
    return context_->height_;
}

float Player::samplerate()
{
    return context_->samplerate_;
}

int32_t Player::channels()
{
    return context_->channels_;
}

int32_t Player::audio_bits()
{
    return context_->audio_bits_;
}

int32_t Player::tracks_count()
{
    return context_->tracks_;
}

int32_t Player::track_current()
{
    return context_->track_;
}

int32_t Player::track_samplerate(int32_t index)
{
    return 0;
}

int32_t Player::track_channels(int32_t index)
{
    return 0;
}

int32_t Player::set_track(int32_t index)
{
    context_->graph->execute_command_async(kGraphCommandCallLuaFunction,Arguments("nativeSetTrack").add(index));
    return 0;
}

int32_t Player::set_channel_mode(int32_t channel_mode)
{
    context_->graph->execute_command_async(kGraphCommandCallLuaFunction,Arguments("nativeSetChannelMode").add(channel_mode));
    return 0;
}

int32_t Player::set_video_emit_mode(bool pull_push)
{
    context_->graph->execute_command_async(kGraphCommandCallLuaFunction,Arguments("nativeSetVideoEmitMode").add(pull_push));
    return 0;
}


}//namespace mr::sdmp
}//namespace mountain-ripper


