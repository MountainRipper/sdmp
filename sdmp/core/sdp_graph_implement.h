#ifndef GRAPHIMPLEMENT_H
#define GRAPHIMPLEMENT_H
#include <thread>
#include "sdpi_graph.h"
#include "sdp_filter_links_flow.h"
#include "sol_lua_operator.h"
#include <eventpp/eventqueue.h>
namespace mr::sdmp {

#define kGraphAsyncExecuteCommandEvent 1

class GraphImplement : public IGraph
{
public:
    GraphImplement(IGraphEvent *event);
    virtual ~GraphImplement();

    int32_t create(const std::string& root_dir,const std::string& script);
public:
    // Graph interface
    virtual std::shared_ptr<sol::state> vm();

    virtual int32_t set_property(const std::string& property,Value& value);
    virtual int32_t get_property(const std::string& property,Value& value);

    virtual int32_t execute_command(const std::string& command,const Arguments& param);
    virtual int32_t execute_command_async(const std::string& command,const Arguments& param);

    virtual int32_t do_master_events();
    virtual int32_t in_master_loop();

    virtual int32_t do_connect(IFilter* sender, IFilter* receiver, int32_t sender_pin_index, int32_t receiver_pin_index);

    virtual const std::map<std::string, FilterPointer> &filters();
    virtual int32_t create_filter(const std::string& id, const Value &filter_config);
    virtual int32_t remove_filter(const std::string& id);

    virtual int32_t emit_error(const std::string& objectId, int32_t code, bool to_script = true);

private:
    int32_t cmd_connect();
    int32_t cmd_disconnect();
    int32_t cmd_play();
    int32_t cmd_pause();
    int32_t cmd_stop();
    int32_t cmd_seek(int64_t millisecond);
    int32_t cmd_close();

    int32_t do_disconnet_sender(IFilter* receiver,int32_t receiver_pin_index = 0);
    int32_t do_disconnet_receiver(IFilter* receiver,int32_t receiver_pin_index = 0);
    int32_t do_disconnet_all();

    int32_t execute_command_lua(const std::string& command,sol::variadic_args args);
    int32_t create_filter_lua(const std::string& id, const sol::lua_value &filter_config);
    int32_t set_filter_property_lua(const std::string& filter_id,const std::string& property,const sol::lua_value& value);
    sol::lua_value call_filter_method_lua(const std::string& filter_id,const std::string &method, sol::variadic_args args);
private:
    int32_t master_thread_proc();
    int32_t master_requare_shot();
    //filters manage
    int32_t create_filters();

    //status manage
    int32_t check_graph_connectings();
    int32_t workflow_execute_command(const std::string &command, const Value &param);
    int32_t switch_status(GraphStatus status);

    sol::table make_features_table();
private:
    std::string root_dir_;
    std::string script_file_;    
    sol::LuaOperator graph_vm_;
    sol::table  graph_context_;
    std::thread master_thread_;    

    std::map<std::string,Value> properties_;
    std::map<std::string ,FilterPointer> filters_;
    FilterLinksFlow filters_flow_;

    bool        quit_flag_        = false;
    int32_t     loop_interval_ms_ = 10;
    GraphStatus status_           = kStatusNone;

    IGraphEvent  *event_           = nullptr;

    sol::protected_function  lua_created_function_;
    sol::protected_function  lua_error_function_;
    sol::protected_function  lua_master_function_;
    sol::protected_function  lua_status_function_;
    sol::protected_function  lua_connect_function_;
    sol::protected_function  lua_connect_done_function_;
    sol::protected_function  lua_position_function_;

    int32_t     current_pts_      = -1;

    FeatureMap  features_;

    eventpp::EventQueue<int32_t, void (GraphImplement* , const std::string &, const Arguments&)> async_queue_;
};

}


#endif // GRAPHIMPLEMENT_H
