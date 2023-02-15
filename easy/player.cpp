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


}//namespace mr::sdmp
}//namespace mountain-ripper
