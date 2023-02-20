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
    PlayerPrivateContex(){}
    std::shared_ptr<sdmp::IGraph> graph;

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
        return 0;
    }
    // Observer interface
public:
    virtual int32_t proxy_render_frame(std::shared_ptr<Frame> frame) override
    {
        MP_INFO("proxy_render_frame");
    }
private:
    int32_t lua_call_native(){
        return 0;
    }
};

Player::Player(const std::string& base_scipts_dir, const std::string& easy_scipts_dir)
{
    sdmp::Factory::initialize_factory();
    sdmp::FeatureMap features;
    sdmp::Factory::initialnize_engine(base_scipts_dir,std::filesystem::path(base_scipts_dir)/"engine.lua",features);

    context_ = new PlayerPrivateContex();
    context_->graph = sdmp::Factory::create_graph_from(std::filesystem::path(easy_scipts_dir)/"player.lua",
                                                       static_cast<sdmp::IGraphEvent*>(context_));

}

int32_t Player::stop()
{
    context_->graph->execute_command(kGraphCommandStop);
    return 0;
}


}//namespace mr::sdmp
}//namespace mountain-ripper


