#ifndef SDP_GRAPH_H
#define SDP_GRAPH_H
#include <string>
#include <functional>
#include "sdpi_filter.h"
#include "sdpi_filter_extentions.h"

namespace mr::sdmp {

//graph event as (IGraph* graph,GraphEventType event,int32_t param)
typedef std::function<int32_t(sdmp::IGraph*,GraphEventType,int32_t)> GraphEvent;

class IGraph{
public:
    virtual ~IGraph(){}
    virtual std::shared_ptr<sol::state> vm() = 0;

    virtual int32_t set_property(const std::string& property,Value& value) = 0;
    virtual int32_t get_property(const std::string& property,Value& value) = 0;

    virtual int32_t execute_command(const std::string& command,const Arguments& param = Arguments()) = 0;
    virtual int32_t execute_command_async(const std::string& command,const Arguments& param = Arguments()) = 0;

    virtual int32_t do_master_events() = 0;
    virtual int32_t in_master_loop() = 0;

    virtual int32_t do_connect(IFilter* sender, IFilter* receiver, int32_t sender_pin_index, int32_t receiver_pin_index) = 0;

    virtual const std::map<std::string, FilterPointer> &filters() = 0;
    virtual int32_t create_filter(const std::string& id,const Value &filter) = 0;
    virtual int32_t remove_filter(const std::string& id) = 0;

    virtual int32_t emit_error(const std::string& objectId, int32_t code, bool to_script = true) = 0;
};

class GraphHelper{
public:
    static int32_t append_video_observer(IGraph* graph,
                                         const std::string& video_output_filter,
                                         IFilterExtentionVideoOutputProxy::Observer* observer,
                                         bool append_remove,
                                         ComPointer<IFilterExtentionVideoOutputProxy> &filter_used);
    static FilterPointer get_filter(IGraph* graph,const std::string& filter_id);
    static int32_t get_filter_property(IGraph* graph,
                                const std::string& filter_id,
                                const std::string& property,
                                Value& value);
    static int32_t set_filter_property(IGraph* graph,
                                const std::string& filter_id,
                                const std::string& property,
                                Value& value);
};
}

#define SDMP_GRAPH_GET_CONTEXT(G) ((*G->vm())[kGraphKey])

#endif // SDP_GRAPH_H
