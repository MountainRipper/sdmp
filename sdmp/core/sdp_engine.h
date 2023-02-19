#ifndef ENGINE_H
#define ENGINE_H
#include <set>
#include "sdpi_graph.h"
#include "core_includes.h"
#include "sdpi_filter_extentions.h"
#include "sol_lua_operator.h"
namespace mr::sdmp {

class IFilterExtentionAudioOutputParticipant;

class Engine : public IGraph
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
    // IGraph interface
public:
    virtual std::shared_ptr<sol::state> vm() override{return engine_state_;}
    virtual int32_t set_property(const std::string &property, Value &value) override{return 0;}
    virtual int32_t get_property(const std::string &property, Value &value) override{return 0;}
    virtual int32_t execute_command(const std::string &command, const Arguments &param) override{return 0;}
    virtual int32_t execute_command_async(const std::string &command, const Arguments &param) override{return 0;}
    virtual int32_t do_master_events() override{return 0;}
    virtual int32_t in_master_loop() override{return 0;}
    virtual int32_t do_connect(IFilter *sender, IFilter *receiver, int32_t sender_pin_index, int32_t receiver_pin_index) override{return 0;}
    virtual const std::map<std::string, FilterPointer> &filters() override{
        static std::map<std::string, FilterPointer> empty;
        return empty;
    }
    virtual int32_t create_filter(const std::string &id, const Value &filter) override{return 0;}
    virtual int32_t remove_filter(const std::string &id) override{return 0;}
    virtual int32_t emit_error(const std::string &objectId, int32_t code, bool to_script) override{return 0;}

private:
    sol::LuaOperator engine_state_;
    std::string root_dir_;
    std::map<std::string,FilterPointer> audio_outputs_devices_;
    std::set<IFilterExtentionAudioOutputParticipant*> audio_output_participant_;
};

}
#endif // ENGINE_H
