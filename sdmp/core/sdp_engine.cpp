#include "sdp_engine.h"
#include "sol/sol.hpp"
#include "sdp_factory_implement.h"

namespace mr::sdmp {

Engine::Engine(const std::string &root_dir, const std::string& script)
{
    root_dir_ = root_dir;
    engine_state_.create();
    engine_state_.state()["rootPath"] = root_dir;
    engine_state_.apply_script(script);
}

Engine::~Engine()
{

}

int32_t Engine::init()
{
    sol::state& vm = engine_state_;
    sol::optional<sol::table> config = vm["audioOutputs"];
    if(config == sol::nullopt)
        return -1;

    auto engine_table = vm.create_table_with("context",this);
    vm["engine"] = engine_table;

    sol::table& audio_outputs = config.value();   
    for(auto item : audio_outputs){
        if(item.first.get_type() != sol::type::string)
            continue;
        if(item.second.get_type() != sol::type::table)
            continue;
        std::string id = item.first.as<std::string>();
        sol::table config = item.second.as<sol::table>();
        config["id"] = id;
        create_device(config);
    }
    return 0;
}

Format Engine::audio_output_format(const std::string &output_id)
{
    Format format;
    auto it = audio_outputs_devices_.find(output_id);
    if(it != audio_outputs_devices_.end()){
        auto filter = it->second;
        auto pin = filter->get_pin(kInputPin,0);

    }
    return format;
}

FilterPointer Engine::get_audio_output_filter(const std::string &output_id)
{
    auto it = audio_outputs_devices_.find(output_id);
    if(it != audio_outputs_devices_.end())
        return it->second;
    return nullptr;
}

int32_t Engine::create_device(sol::table& config)
{
    std::string module = config["module"].get_or(std::string("miniaudioOutput"));
    std::string id = config["id"].get_or(std::string(""));
    if(id.empty())
        return -1;

    MP_LOG_DEAULT("Engine::create_device id: {}, module: {}", id.data(), module.data());

    FilterPointer device_filter = Factory::factory()->create_filter(module);
    MP_LOG_DEAULT("created device filter: {}", (void*)device_filter.Get());
    int32_t result = device_filter->initialize(nullptr,config);
    if(result >= 0){
        audio_outputs_devices_[id] = device_filter;
        return 0;
    }

    return -1;
}


}
