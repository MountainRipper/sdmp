#include <sol/sol.hpp>
#include "player.h"
#include "../sdmp/interface/sdpi_factory.h"

namespace mr {
namespace sdmp {

class PlayerPrivateContex{
public:
    PlayerPrivateContex(){}
    std::shared_ptr<sdmp::IGraph> graph;
};

Player::Player(const std::string& base_scipts_dir, const std::string& easy_scipts_dir)
{
    sdmp::Factory::initialize_factory();
    sdmp::FeatureMap features;
    sdmp::Factory::initialnize_engine(base_scipts_dir,"engine.lua",features);

    context_ = new PlayerPrivateContex();
}

int32_t Player::lua_call(const std::string &name, Value &value)
{
    return 0;
}

int32_t Player::on_graph_init(IGraph *graph)
{
    auto& vm = *graph->vm();
    vm["nativeContext"] = this;
    vm["nativeCall"] = &Player::lua_call;
    return 0;
}

int32_t Player::on_graph_created(IGraph *graph)
{
    return 0;
}

int32_t Player::on_graph_error(IGraph *graph, int32_t error_code)
{
    return 0;
}

int32_t Player::on_graph_master_loop(IGraph *graph)
{
    return 0;
}


}//namespace mr::sdmp
}//namespace mountain-ripper


