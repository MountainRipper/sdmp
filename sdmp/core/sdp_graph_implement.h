#ifndef GRAPHIMPLEMENT_H
#define GRAPHIMPLEMENT_H
#include <thread>
#include "sdpi_graph.h"
#include "sdp_filter_links_flow.h"
#include "sol_lua_operator.h"
namespace mr::sdmp {

class GraphImplement : public IGraph
{
public:
    GraphImplement(IGraphEvent *event);
    virtual ~GraphImplement();

    int32_t create(const std::string& root_dir,const std::string& script);
public:
    // Graph interface
    virtual int32_t destory();
    virtual std::shared_ptr<sol::state> vm();
    virtual sol::table& graph();

    virtual int32_t set_property(const std::string& property,Value& value);
    virtual int32_t get_property(const std::string& property,Value& value);

    virtual int32_t cmd_connect();    
    virtual int32_t cmd_play();
    virtual int32_t cmd_pause();
    virtual int32_t cmd_stop();
    virtual int32_t cmd_seek(int64_t millisecond);
    virtual int32_t cmd_close();
    virtual int32_t do_master_events();
    virtual int32_t in_master_loop();

    //NOTE: connect/disconnect index from 1 (for lua compatible)
    virtual int32_t do_connect(IFilter* sender, IFilter* receiver, int32_t sender_pin_index, int32_t receiver_pin_index);
    virtual int32_t do_disconnet_sender(IFilter* receiver,int32_t receiver_pin_index = 0);
    virtual int32_t do_disconnet_receiver(IFilter* receiver,int32_t receiver_pin_index = 0);
    virtual int32_t do_disconnet_all();    

    virtual const std::map<std::string, FilterPointer> &filters();
    virtual int32_t create_filter(const std::string& id,const sol::table &filter);
    virtual int32_t remove_filter(const std::string& id);

    virtual int32_t emit_error(const std::string& objectId, int32_t code, bool to_script = true);
private:
    int32_t master_thread_proc();
    int32_t master_requare_shot();

    //filters manage
    int32_t create_filters();
    int32_t do_disconnect();
    int32_t check_graph_connectings();

    //status manage
    int32_t do_flow_command(const std::string &command, const Value &param);
    int32_t notify_cmd();
    int32_t switch_status(GraphStatus status);

    sol::table make_features_table();
private:

private:
    std::string root_dir_;
    std::string script_file_;    
    LuaOperator graph_vm_;
    sol::table  graph_context_;
    std::thread master_thread_;    

    SolPropertiesMap properties_;
    std::map<std::string ,FilterPointer> filters_;
    FilterLinksFlow filters_flow_;

    bool        quit_flag_        = false;
    int32_t     loop_interval_ms_ = 10;
    GraphStatus      status_           = kStatusNone;

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
};

}


#endif // GRAPHIMPLEMENT_H
