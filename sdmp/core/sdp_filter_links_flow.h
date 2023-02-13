#ifndef AUDIOOUTPUTPARTICIPANTFILTER_H
#define AUDIOOUTPUTPARTICIPANTFILTER_H

#include <sdp_general_filter_base.h>
#include "sdp_timeline_support.h"

namespace sdp {

struct FilterNode{
    FilterPointer filter;
    std::vector<PinIndex> input_pins_;
    std::vector<PinIndex> output_pins_;
};

struct FilterTreeLinks{
    FilterPointer origin = nullptr;
    std::vector<std::vector<FilterNode>> links;
    bool eos = false;
};

class FilterLinksFlow
{
public:
    FilterLinksFlow();
    int32_t reset();
    int32_t create_from_graph(IGraph* graph);
    int32_t request_flow_stream_shot();
    int32_t switch_status(GraphStatus status);
    int32_t process_command(const std::string &command,const NativeValue& param);
    int64_t timeline(bool in_linear = true);
private:
    int32_t push_activale_link(std::vector<FilterPointer> &link);
    int32_t check_link_eos(std::vector<FilterNode>& link);
    int32_t disable_inactivable_filters();
private:
    IGraph* graph_ = nullptr;
    std::vector<std::vector<FilterPointer>> activable_links_;
    std::set<FilterPointer>  activable_filters_;
    std::set<FilterPointer>  inactivable_filters_;

    std::vector<FilterTreeLinks> source_filter_tree_;
    std::vector<FilterTreeLinks> render_filter_tree_;
    std::map<int32_t,std::set<FilterPointer>> render_level_filters_;

    TimelineDecisionGroup timeline_decision_;
};

}

#endif // FILTERLINKSFLOW_H
