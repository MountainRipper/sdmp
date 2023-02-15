#include <sstream>
#include "sdp_filter_links_flow.h"
#include "sdp_filter_helper.h"
#include "sdpi_graph.h"
#include "logger.h"
namespace mr::sdmp {

FilterLinksFlow::FilterLinksFlow()
{

}

int32_t FilterLinksFlow::reset()
{
    activable_filters_.clear();
    activable_links_.clear();
    inactivable_filters_.clear();
    source_filter_tree_.clear();
    render_filter_tree_.clear();
    render_level_filters_.clear();
    timeline_decision_.clear_timeline();
    return 0;
}

int32_t FilterLinksFlow::create_from_graph(IGraph *graph)
{
    graph_ = graph;

    reset();

    auto& filters = graph->filters();
    std::set<FilterPointer> source_filters;
    for(auto filter : filters){
        filter.second->set_level(kFilterLevelInvalid);
        auto type = filter.second->type();
        if((type & kFilterTypeAudioInput) || (type & kFilterTypeVideoInput)){
            source_filters.insert(filter.second);
        }
    }

    std::vector<std::vector<FilterPointer>> links;
    for(auto item : source_filters){
        MP_LOG_DEAULT(">>Find source filter:{}",item->id().c_str());
        FilterHelper::get_filter_receiver_links(item,links);
    }

    MP_LOG_DEAULT(">>Connecting check get filter links:");
    for (auto& link : links) {

        if(link.empty())
            continue;
        auto tail_filter_type = link.back()->type();
        if(FILTER_IS_OUTPUT(tail_filter_type) == false){
            MP_LOG_DEAULT("INVALID: ");
            FilterHelper::print_filter_list(link,"->");
            continue;
        }

        push_activale_link(link);

        MP_LOG_DEAULT("VALID: ");
        FilterHelper::print_filter_list(link,"->");
    }

    for(auto item : filters){
        auto filter = item.second.Get();
        if(activable_filters_.find(filter) == activable_filters_.end()){
            filter->process_command(kFilterCommandInactive,.0);
            inactivable_filters_.insert(filter);
        }
    }
    MP_LOG_DEAULT(">>Connecting check get inactivable filters:");
    std::vector<FilterPointer> temp(inactivable_filters_.begin(),inactivable_filters_.end());
    FilterHelper::print_filter_list(temp,",");

    disable_inactivable_filters();
    return 0;
}


int32_t FilterLinksFlow::request_flow_stream_shot()
{    
    int32_t ret = 0;
    //master loop call to every active filter
    for(auto& filter : activable_filters_){
        filter->master_loop(true);
    }

    Value value;
    int32_t eos_tree_count = 0;
    for(auto& tree : render_filter_tree_){
        if(tree.eos){
            eos_tree_count++;
            continue;
        }
        int32_t eos_links = 0;
        auto& links = tree.links;
        for(auto& link : links){

            int32_t requare_ret = 0;
            int32_t count = link.size();
            while (count>0) {
                count--;

                FilterPointer& filter = link[count].filter;
                auto& pin_indexs = link[count].output_pins_;
                requare_ret = filter->requare(requare_ret,pin_indexs);

                if(requare_ret > 0){
                    continue;
                }
                else if(requare_ret == 0){
                    //current filter send enough stream,don't need sender filter to deliver more
                    break;
                }
                else if(requare_ret == kErrorFilterEos){
                    //current filter eos,don't need sender filter to deliver more
                    break;
                }
                else if(requare_ret < 0){
                    //other errors in requare call
                    break;
                }
            }

            FilterPointer& render_filter = link.back().filter;

            render_filter->get_property(kFilterPropertyStatus,value);
            if(int(value) == kStatusEos){
                eos_links++;
            }
        }
        if(eos_links == links.size()){
            tree.eos = true;
            eos_tree_count++;
        }
    }

    //decision pts
    timeline_decision_.calculation_current_timeline();

    if(eos_tree_count == render_filter_tree_.size()){
        MP_LOG_DEAULT(">>flow checked all filter trees eos:");
        switch_status(kStatusEos);
        ret = kStatusEos;
    }

    //master loop call to every active filter
    for(auto& filter : activable_filters_){
        filter->master_loop(false);
    }
    return ret;
}

int32_t FilterLinksFlow::push_activale_link(std::vector<FilterPointer> &link)
{
    ComPointer<IFilter> render = link.back();
    auto source = link.front();
    auto tail_filter_type = render->type();

    //try get timeline support from render
    auto timeline = render.As<IFilterExtentionTimeline>();
    if(timeline)
        timeline_decision_.append_timeline(timeline.Get());

    //put audio links first
    if(tail_filter_type & kFilterTypeAudioOutput)
        activable_links_.insert(activable_links_.begin(),link);
    else if(tail_filter_type & kFilterTypeVideoOutput)
        activable_links_.push_back(link);

    for (auto filter : link) {
        filter->process_command(kFilterCommandActive,.0);
        activable_filters_.insert(filter);
    }

    //get source link tree
    FilterTreeLinks* source_link = nullptr;
    for(auto& item : source_filter_tree_){
        if(item.origin == source){
            source_link = &item;
            break;
        }
    }
    if(source_link == nullptr){
        FilterTreeLinks new_link;
        new_link.origin = source;
        source_filter_tree_.push_back(new_link);
        source_link = &source_filter_tree_.back();
    }

    //get render link tree
    FilterTreeLinks* render_link = nullptr;
    for(auto& item : render_filter_tree_){
        if(item.origin == render){
            render_link = &item;
            break;
        }
    }
    if(render_link == nullptr){
        FilterTreeLinks new_link;
        new_link.origin = render;

        if(tail_filter_type & kFilterTypeAudioOutput){
            render_filter_tree_.insert(render_filter_tree_.begin(),new_link);
            render_link = &render_filter_tree_.front();
        }
        else if(tail_filter_type & kFilterTypeVideoOutput){
            render_filter_tree_.push_back(new_link);
            render_link = &render_filter_tree_.back();
        }
    }

    //parse to node with pin index
    std::vector<FilterNode> node_link;

    for(auto item : link){
        FilterNode node;
        node.filter = item;
        node_link.push_back(node);
    }
    int32_t count = link.size();
    count-=1;
    for(int32_t index = 0; index < count; index++){
        FilterHelper::get_filter_linked_pin(link[index],link[index+1],node_link[index].output_pins_,node_link[index+1].input_pins_);
    }

    source_link->links.push_back(node_link);
    render_link->links.push_back(node_link);

    int32_t level = -1;
    std::for_each(node_link.rbegin(),node_link.rend(),[&level,this](FilterNode& node){
        ++level;
        auto filter = node.filter;
        auto level_cur = filter->level();
        if(level_cur != kFilterLevelInvalid){
            if(level_cur > level){
                render_level_filters_[level_cur].erase(filter);
            }
            else
                return;
        }
        filter->set_level(level);
        if(render_level_filters_.find(level) == render_level_filters_.end())
            render_level_filters_[level] = std::set<FilterPointer>();
        render_level_filters_[level].insert(filter);
    });
    return 0;
}

int32_t FilterLinksFlow::check_link_eos(std::vector<FilterNode> &link)
{
    int32_t count = link.size();
    count -= 1;
    Value value;
    int32_t eos_count = 0;
    for(int32_t index = 0;index < count;index++){
        FilterPointer filter = link[index].filter;
        filter->get_property(kFilterPropertyStatus,value);
        GraphStatus status = GraphStatus((int)value);
        if(status == kStatusEos)
            eos_count++;
    }
    if(eos_count == count){
        auto& input_pins = link.back().input_pins_;
        for(auto& pin : input_pins ){
            value = (int)pin.index;
            link.back().filter->set_property("",value);
        }
    }
    return 0;
}

int32_t FilterLinksFlow::switch_status(GraphStatus status)
{
    Value value(status);
    for(auto filter : activable_filters_){
        filter->set_property(kFilterPropertyStatus,value);
    }
    return 0;
}

int32_t FilterLinksFlow::process_command(const std::string &command, const Value &param)
{
    bool forward_direction = true;
    if(command == kGraphCommandSeek || command == kGraphCommandPlay){
        if(render_filter_tree_.empty()){
            MP_ERROR("there is no valid filter links to run");
            return -1;
        }
        //pass command from receiver to sender, so receiver can prepair for status.
        forward_direction = false;
        for(auto& tree : render_filter_tree_)
            tree.eos = false;
    }

    int ret = 0;
    FilterPointer filter_suspend = nullptr;

    auto chain_function = [&](std::map<int32_t,std::set<FilterPointer>> ::reference item){
        auto& filters = item.second;
        for(auto& filter : filters){
            //ingore when process none-stop command faild
            if(command != kGraphCommandStop && ret < 0)
                return;
            MP_LOG_DEAULT("filter:{} level:{} process_command:{}",filter->id().c_str(),filter->level(),command.c_str());
            ret = filter->process_command(command,param);
            if (command != kGraphCommandStop  && ret < 0) {
                filter_suspend = filter;
                break;
            }
        }
    };

    if(forward_direction)
        std::for_each(render_level_filters_.rbegin(),render_level_filters_.rend(),chain_function);
    else
        std::for_each(render_level_filters_.begin(),render_level_filters_.end(),chain_function);

    if(filter_suspend)
    {
        MP_ERROR("FilterLinksFlow::process_command '{}' failed:{} and suspend, at filter [{}]",
                  command.c_str(),
                  ret,
                  filter_suspend->id().c_str());
    }
    return ret;
}

int64_t FilterLinksFlow::timeline(bool in_linear)
{
    if(in_linear)
        return timeline_decision_.timeline_current_in_linear();
    else
        return timeline_decision_.timeline_current();
}

int32_t FilterLinksFlow::disable_inactivable_filters()
{
    Value value(kStatusStoped);
    for(auto filter : inactivable_filters_){
        filter->set_property(kFilterPropertyStatus,value);
    }
    return 0;
}


}
