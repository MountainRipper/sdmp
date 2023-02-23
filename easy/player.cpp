#include <filesystem>
#include <sol/sol.hpp>
#include <sdmp/interface/sdpi_factory.h>
#include <sdmp/interface/sdpi_filter_extentions.h>
#include "logger.h"
#include "player.h"

namespace mr {
namespace sdmp {

class PlayerPrivateContex
        : public IGraphEvent
        , public IFilterExtentionVideoOutputProxy::Observer{
public:
    PlayerPrivateContex(Player* player)
        :player_(player){

    }
    std::shared_ptr<sdmp::IGraph> graph;

    int32_t set_event(PlayerEvent* event){
        event_ = event;
        return 0;
    }
    // IGraphEvent interface
public:
    virtual int32_t on_graph_init(IGraph *graph) override
    {
        sol::state& state = *graph->vm();
        state["luaCallNative"] = &PlayerPrivateContex::lua_call_native;
        state["nativeContext"] = this;
        return 0;
    }
    virtual int32_t on_graph_created(IGraph *graph) override
    {        
        ComPointer<IFilterExtentionVideoOutputProxy> render;
        sdmp::GraphHelper::append_video_observer(graph,"",static_cast<IFilterExtentionVideoOutputProxy::Observer*>(this),true,render);
        graph->execute_command(kGraphCommandConnect);
        graph->execute_command(kGraphCommandPlay);
        return 0;
    }
    virtual int32_t on_graph_error(IGraph *graph, int32_t error_code) override
    {
        return 0;
    }
    virtual int32_t on_graph_master_loop(IGraph *graph) override
    {
        Value value;
        graph->get_property(kGraphPropertyCurrentPts,value);
        if(event_)
            event_->on_position(player_,value.as_int32());
        return 0;
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
        if(type == "connect-done"){
            const sol::table& graph_tb = SDMP_GRAPH_GET_CONTEXT(graph);
            std::string uri = graph_tb["info"]["uri"];
            double duration = graph_tb["info"]["duration"];
            duration_ = std::round(duration);
        }
        return 0;
    }
public:
    Player* player_ = nullptr;
    PlayerEvent* event_ = nullptr;
    int64_t duration_ = 0;
};

Player::Player(const std::string& base_scipts_dir, const std::string& easy_scipts_dir)
{
    sdmp::Factory::initialize_factory();
    sdmp::FeatureMap features;
    sdmp::Factory::initialnize_engine(base_scipts_dir,std::filesystem::path(base_scipts_dir)/"engine.lua",features);

    context_ = new PlayerPrivateContex(this);
    context_->graph = sdmp::Factory::create_graph_from(std::filesystem::path(easy_scipts_dir)/"player.lua",
                                                       static_cast<sdmp::IGraphEvent*>(context_));

    set_video_emit_mode(true);
}

int32_t Player::set_event(PlayerEvent *event)
{
    context_->set_event(event);
    return 0;
}

int32_t Player::play(int32_t position_ms)
{
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
    context_->graph->execute_command_async(kGraphCommandPlay);
    return 0;
}

int32_t Player::stop()
{
    context_->graph->execute_command_async(kGraphCommandStop);
    return 0;
}

int32_t Player::duration()
{
    return context_->duration_;
}

int32_t Player::set_video_emit_mode(bool pull_push)
{
    //context_->graph->execute_command_async(kGraphCommandCallLuaFunction,Arguments("setVideoEmitMode").add(pull_push));
    return 0;
}


}//namespace mr::sdmp
}//namespace mountain-ripper


