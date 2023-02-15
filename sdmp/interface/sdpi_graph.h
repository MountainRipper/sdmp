#ifndef SDP_GRAPH_H
#define SDP_GRAPH_H
#include <string>
#include "sdpi_filter.h"
#include "sdpi_filter_extentions.h"

namespace mr::sdmp {

class IGraphEvent{
public:
    virtual ~IGraphEvent(){}
    virtual int32_t on_graph_init(sdmp::IGraph *graph) = 0;
    virtual int32_t on_graph_created(IGraph* graph) = 0;
    virtual int32_t on_graph_error(IGraph* graph,int32_t error_code) = 0;
    virtual int32_t on_graph_master_loop(IGraph* graph) = 0;
};

class IGraph{
public:
    virtual ~IGraph(){}
    virtual std::shared_ptr<sol::state> vm() = 0;
    virtual sol::table& graph() = 0;

    virtual int32_t set_property(const std::string& property,Value& value) = 0;
    virtual int32_t get_property(const std::string& property,Value& value) = 0;

    virtual int32_t cmd_connect() = 0;
    virtual int32_t cmd_play() = 0;
    virtual int32_t cmd_pause() = 0;
    virtual int32_t cmd_stop() = 0;
    virtual int32_t cmd_seek(int64_t millisecond) = 0;
    virtual int32_t cmd_close() = 0;

    virtual int32_t do_master_events() = 0;
    virtual int32_t in_master_loop() = 0;

    virtual int32_t do_connect(IFilter* sender, IFilter* receiver, int32_t sender_pin_index, int32_t receiver_pin_index) = 0;

    virtual const std::map<std::string, FilterPointer> &filters() = 0;
    virtual int32_t create_filter(const std::string& id,const sol::table &filter) = 0;
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
#endif // SDP_GRAPH_H
