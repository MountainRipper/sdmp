#ifndef ENGINE_H
#define ENGINE_H
#include <set>
#include "sdpi_graph.h"
#include "core_includes.h"
#include "sdpi_filter_extentions.h"
namespace sdp {

class IFilterExtentionAudioOutputParticipant;

class Engine
{
public:
    Engine(const std::string& root_dir, const std::string &script);
    ~Engine();
    int32_t init();

    Format  audio_output_format(const std::string& output_id);
    FilterPointer get_audio_output_filter(const std::string& output_id);
    int32_t remove_audio_output_filter(IFilterExtentionAudioOutputParticipant* filter,const std::string& output_id);
private:
    int32_t create_device(sol::table &config);
private:
    LuaOperator engine_state_;
    std::string root_dir_;
    std::map<std::string,FilterPointer> audio_outputs_devices_;
    std::set<IFilterExtentionAudioOutputParticipant*> audio_output_participant_;
};

}
#endif // ENGINE_H
