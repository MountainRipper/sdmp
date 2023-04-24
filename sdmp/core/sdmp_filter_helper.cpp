#include <functional>
#include <set>
#include <sstream>
#include <mr/logger.h>
#include <sdmpi_pin.h>
#include <sdmpi_filter.h>
#include "tree_recursive.h"

namespace mr::sdmp {

using namespace mr::sdmp::commom;


int32_t enum_receivers(TreeNode<FilterPointer>& node,std::set<FilterPointer>& all_filters){
    std::set<FilterPointer> receivers = FilterHelper::get_filter_receivers(node.value_);
    for(auto filter : receivers){
        if(all_filters.find(filter) != all_filters.end()){
            MR_WARN("enum_receivers tree find a multi linked filter {}, if realy multi input pin from multi link, ingore this message",filter->id());
            //return -1;
        }
        TreeNode<FilterPointer> child_node(filter);
        node.childrens_.push_back(child_node);
        all_filters.insert(filter);
        int ret = enum_receivers(node.childrens_.back(),all_filters);
        if(ret < 0)
            return ret;
    }
    return 0;
};

int32_t get_filter_receiver_tree(sdmp::commom::TreeNode<FilterPointer>& root)
{
    std::set<FilterPointer> all_filters;
    return enum_receivers(root,all_filters);
}


int32_t FilterHelper::print_filter_list(const std::vector<FilterPointer> &list, const string &spliter)
{
    std::stringstream stream;
    stream<<"[";
    int32_t index = 0;
    for (auto filter : list) {
        if((index++)>0)
            stream<<spliter;
        stream<<filter->id();
    }
    stream<<"]";
    MR_LOG_DEAULT(stream.str());
    return 0;
}

std::set<FilterPointer> FilterHelper::get_filter_near(sdmp::FilterPointer filter, PinDirection direction)
{
    std::set<FilterPointer> filters_near;
    int index = 0;
    while (true) {
        auto pin = filter->get_pin(direction,index++);
        if(pin != nullptr){
            if(direction == kOutputPin){
                auto receivers = pin->receivers();
                for(auto receiver : receivers){
                    auto near_filter = receiver->filter();
                    if(near_filter)
                        filters_near.insert(near_filter);
                }
            }
            else{
                auto sender = pin->sender();
                auto filter = sender->filter();
                if(sender && filter)
                    filters_near.insert(filter);
            }
        }
        else
            break;
    }
    return filters_near;
}

std::set<FilterPointer> FilterHelper::get_filter_senders(FilterPointer filter)
{
    return get_filter_near(filter,kInputPin);
}

std::set<FilterPointer> FilterHelper::get_filter_receivers(FilterPointer filter)
{
    //NOTE:output filter will not has receiver(break for internal engin filter connect)
    auto type = filter->type();
    if(FILTER_IS_OUTPUT(type))
        return std::set<FilterPointer>();

    return get_filter_near(filter,kOutputPin);
}

int32_t FilterHelper::get_filter_receiver_links(FilterPointer filter,std::vector<std::vector<FilterPointer> >& links)
{
    sdmp::commom::TreeNode<FilterPointer> root(filter);
    int32_t ret = get_filter_receiver_tree(root);
    if(ret < 0)
        return ret;
    TreeRecursive::recurTree(root,links);
    return 0;
}

int32_t FilterHelper::get_filter_linked_pin(FilterPointer sender, FilterPointer receiver, std::vector<PinIndex> &sender_pins, std::vector<PinIndex> &receiver_pins)
{
    for(int sender_index = 0; ; sender_index++ ){
        auto send_pin = sender->get_pin(kOutputPin,sender_index);
        if(send_pin == nullptr)
            break;

        for(int receiver_index = 0; ; receiver_index++ ){
            auto receive_pin = receiver->get_pin(kInputPin,receiver_index);
            if(receive_pin == nullptr)
                break;
            auto&  sender_receivers = send_pin->receivers();
            if(sender_receivers.find(receive_pin.Get()) != sender_receivers.end()){
                sender_pins.push_back(PinIndex{send_pin.Get(),kOutputPin,sender_index});
                receiver_pins.push_back(PinIndex{receive_pin.Get(),kInputPin,receiver_index});
            }
        }
    }
    return 0;
}


int32_t FilterHelper::connect(sdmp::FilterPointer sender, int32_t output_pin_index, sdmp::FilterPointer receiver, int32_t input_pin_index)
{
    auto output_pin = sender->get_pin(kOutputPin,output_pin_index);
    auto input_pin = receiver->get_pin(kInputPin,input_pin_index);
    if(output_pin==nullptr)
        return kErrorConnectFailedNoOutputPin;
    if(input_pin == nullptr)
        return kErrorConnectFailedNoInputPin;
    if(input_pin->sender() != nullptr)
        return kErrorConnectFailedInputPinAlreadyConnected;

    return sender->connect(output_pin.Get(),input_pin.Get());
}


int32_t FilterHelper::disconnect_output(sdmp::FilterPointer filter, int32_t output_pin)
{
    auto pin = filter->get_pin(kOutputPin,output_pin);
    if(pin == nullptr)
        return kErrorInvalidParameter;

    auto receivers = pin->receivers();
    for(auto recv_pin : receivers){
        filter->disconnect_output(output_pin,recv_pin);
    }
    return 0;
}

int32_t FilterHelper::disconnect_output(sdmp::FilterPointer filter)
{
    auto output_pins = filter->get_pins(kOutputPin);
    for(auto pin : output_pins){
        disconnect_output(filter,pin->index());
    }
    return 0;
}
}
