#include "synthesis.h"

#include <filesystem>
#include <sol/sol.hpp>
#include <sdmpi_factory.h>
#include <sdmpi_filter_extentions.h>
namespace mr {
namespace sdmp {

class SynthesisPrivateContex{
public:
    SynthesisPrivateContex(Synthesis* owner,const std::string& script)
        :owner_(owner){
        graph = sdmp::Factory::create_graph_from(script,[this](sdmp::IGraph* graph,GraphEventType event,int32_t param){
            if(event == kGraphEventLoaded){
                sol::state& state = *graph->vm();
                state["luaCallNative"] = &SynthesisPrivateContex::lua_call_native;
                state["nativeContext"] = this;
                return 0;
            }
            else if(event == kGraphEventCreated){
                // ComPointer<IFilterExtentionVideoOutputProxy> render;
                // sdmp::GraphHelper::append_video_observer(graph,"",static_cast<IFilterExtentionVideoOutputProxy::Observer*>(this),true,render);
                // graph->execute_command(kGraphCommandConnect);
                // graph->execute_command(kGraphCommandPlay);
                // if(event_)
                //     event_->on_inited();
                return 0;
            }
            return 0;
        });
    }

    int32_t lua_call_native(const std::string& type,sol::variadic_args args){
        return 0;
    }

private:
    Synthesis* owner_;
    std::shared_ptr<sdmp::IGraph> graph;
};

Synthesis::Synthesis(const std::string& base_scipts_dir, const std::string& easy_scipts_dir) {

    sdmp::Factory::initialize_factory();
    sdmp::FeatureMap features;
    sdmp::Factory::initialnize_engine(base_scipts_dir,(std::filesystem::path(base_scipts_dir)/"engine.lua").string(),features);

    context_ = new SynthesisPrivateContex(this,(std::filesystem::path(easy_scipts_dir)/"synthesis.lua").string());
}

Synthesis::~Synthesis()
{
    if(context_)
        delete context_;
}


}//namespace mr::sdmp
}//namespace mountain-ripper
