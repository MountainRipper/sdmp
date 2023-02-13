#include <thread>
#include <chrono>
#include "sdp_graph_implement.h"
#include "sdp_factory_implement.h"
#include "sdp_filter_helper.h"
#include "core_includes.h"

int my_exception_handler(lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description) {
    // L is the lua state, which you can wrap in a state_view if necessary
    // maybe_exception will contain exception, if it exists
    // description will either be the what() of the exception or a description saying that we hit the general-case catch(...)
    std::cerr << "An exception occurred in a function, here's what it says ";
    if (maybe_exception) {
        std::cerr << "(straight from the exception): ";
        const std::exception& ex = *maybe_exception;
        std::cerr << ex.what() << std::endl;
    }
    else {
        std::cerr << "(from the description parameter): ";
        std::cerr.write(description.data(), static_cast<std::streamsize>(description.size()));
        std::cerr << std::endl;
    }

    // you must push 1 element onto the stack to be
    // transported through as the error object in Lua
    // note that Lua -- and 99.5% of all Lua users and libraries -- expects a string
    // so we push a single string (in our case, the description of the error)
    return sol::stack::push(L, description);
}

namespace sdp {

GraphImplement::GraphImplement(IGraphEvent *event)
    :event_(event)
{

}

GraphImplement::~GraphImplement()
{
    quit_flag_ = true;
    if(master_thread_.joinable())
        master_thread_.join();    
    MP_INFO("### GraphImplement Release: {}",(void*)this);
}

int32_t GraphImplement::create(const std::string &root_dir, const std::string &script)
{
    if(master_thread_.joinable())
        return -1;
    root_dir_ = root_dir;
    script_file_ = script;
    master_thread_ = std::thread(&GraphImplement::master_thread_proc,this);
    return 0;
}

int32_t GraphImplement::cmd_connect()
{
    MP_LOG_DEAULT("{}",__FUNCTION__);
    lua_connect_function_(graph_context_);
    check_graph_connectings();
    lua_connect_done_function_(graph_context_);
    return 0;
}

int32_t GraphImplement::do_disconnect()
{
    MP_LOG_DEAULT("{}",__FUNCTION__);
    do_disconnet_all();
    // filters_flow_.create_from_graph(this);
    return 0;
}

int32_t GraphImplement::cmd_play()
{
    MP_LOG_DEAULT("{}",__FUNCTION__);
    return do_flow_command(kGraphCommandPlay,.0);
}

int32_t GraphImplement::cmd_pause()
{
    do_flow_command(kGraphCommandPause,.0);
    MP_LOG_DEAULT("{}",__FUNCTION__);
    return 0;
}

int32_t GraphImplement::cmd_stop()
{
    cmd_pause();
    do_flow_command(kGraphCommandStop,.0);
    do_disconnect();
    current_pts_ = kGraphInvalidPts;
    //emit a invalid position manually
    lua_position_function_(graph_context_,(double)current_pts_);
    MP_LOG_DEAULT("{}",__FUNCTION__);
    return 0;
}

int32_t GraphImplement::cmd_seek(int64_t millisecond)
{    
    MP_LOG_DEAULT("{}: {}",__FUNCTION__,millisecond);
    do_flow_command(kGraphCommandPause,millisecond);
    do_flow_command(kGraphCommandSeek,millisecond);
    current_pts_ = kGraphInvalidPts;
    return 0;
}

int32_t GraphImplement::cmd_close()
{
    MP_LOG_DEAULT("{}",__FUNCTION__);
    cmd_stop();
    filters_flow_.reset();
    return 0;
}

int32_t GraphImplement::do_master_events()
{    
    return master_requare_shot();
}

int32_t GraphImplement::in_master_loop()
{
    return std::this_thread::get_id() == master_thread_.get_id();
}

int32_t sdp::GraphImplement::destory()
{
    return 0;
}

std::shared_ptr<sol::state> sdp::GraphImplement::vm()
{
    return graph_vm_;
}

sol::table &GraphImplement::graph()
{
    return graph_context_;
}

int32_t GraphImplement::master_requare_shot()
{
    if(event_)
        event_->on_graph_master_loop(this);
    if(lua_master_function_.valid())
        lua_master_function_(graph_context_);    

    if(status_ != kStatusRunning){
        return 0;
    }

    int32_t ret = filters_flow_.request_flow_stream_shot();
    if(ret == kStatusEos){
        switch_status(kStatusEos);
    }

    if(current_pts_ != kGraphInvalidPts){
        double pts = current_pts_;
        lua_position_function_(graph_context_,pts);
    }
    return 0;
}

int32_t GraphImplement::master_thread_proc()
{
    graph_vm_.create();
    graph_vm_.set_error_handler_function("onScriptError");

    sol::state& vm = graph_vm_;
    vm["rootPath"] = root_dir_;
    vm["hostOS"] = std::string(SDP_OS);
    vm["hostARCH"] = std::string(SDP_ARCH);
    vm["hostFeature"] = make_features_table();
    int32_t ret = graph_vm_.apply_script(script_file_);
    if(ret < 0){
        return emit_error("graph",kErrorBadScript,false);
    }
    //native init notify
    if(event_)
        event_->on_graph_init(this);

    sol::optional<sol::table> graph_opt = vm[kLuaCreateGraphFunctionName]();
    if(graph_opt == sol::nullopt){
        MP_ERROR("wrong graph script: graph create function \"createGraph\" get nil graph;");
        return emit_error("graph",kErrorCreateGraphScriptObject,false);
    }

    graph_context_ = graph_opt.value();

    vm[kGraphKey] = graph_context_;

    graph_context_["context"] = this;    
    graph_context_[kGraphCommandConnect] = &GraphImplement::cmd_connect;
    graph_context_[kGraphCommandPlay] = &GraphImplement::cmd_play;
    graph_context_[kGraphCommandPause] = &GraphImplement::cmd_pause;
    graph_context_[kGraphCommandStop] = &GraphImplement::cmd_stop;
    graph_context_[kGraphCommandSeek] = &GraphImplement::cmd_seek;
    graph_context_[kGraphCommandClose] = &GraphImplement::cmd_close;


    graph_context_[kGraphOperatorConnect] = &GraphImplement::do_connect;
    graph_context_[kGraphOperatorCreateFilter] = &GraphImplement::create_filter;
    graph_context_[kGraphOperatorRemoveFilter] = &GraphImplement::remove_filter;

    loop_interval_ms_   = graph_context_.get_or("loopsInterval",10);

    lua_created_function_       = graph_vm_.get_member_function(graph_context_,kLuaCreatedEventFunctionName);
    lua_error_function_         = graph_vm_.get_member_function(graph_context_,kLuaErrorEventFunctionName);
    lua_master_function_        = graph_vm_.get_member_function(graph_context_,kLuaMasterLoopEventFunctionName);
    lua_status_function_        = graph_vm_.get_member_function(graph_context_,kLuaStatusEventFunctionName);
    lua_connect_function_       = graph_vm_.get_member_function(graph_context_,kLuaConnectEventFunctionName);
    lua_connect_done_function_  = graph_vm_.get_member_function(graph_context_,kLuaConnectDoneEventFunctionName);
    lua_position_function_      = graph_vm_.get_member_function(graph_context_,kLuaPositionEventFunctionName);

    ret = create_filters();
    if(ret < 0){
        return emit_error("graph",kErrorCreateGraphFilterObject);
    }

    //native created notify
    if(event_)
        event_->on_graph_created(this);
    //script created notify
    lua_created_function_(graph_context_);

    //do init connect
    // cmd_connect();

    while (!quit_flag_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(loop_interval_ms_));
//        TIMER_DECLEAR(timer)
        master_requare_shot();
//        TIMER_PRINT_ELAPSED(timer,"master_requare_shot running:");
    }
    cmd_pause();
    cmd_close();

    filters_.clear();
    if(ret < 0)
        return ret;
    return 0;
}

int32_t GraphImplement::emit_error(const std::string &objectId, int32_t code, bool to_script)
{
    if(event_)
        event_->on_graph_error(this,code);

    if(lua_error_function_)
        lua_error_function_(graph_context_,objectId,code);

    return code;
}

int32_t GraphImplement::create_filters()
{
    sol::optional<sol::table> filters_opt = graph_context_["filters"];
    if(filters_opt == sol::nullopt){
        MP_ERROR("wrong graph script: no graph.filters table defined;");
        return -1;
    }

    sol::table filters = filters_opt.value();

    int32_t ret = 0;
    //for(int index = 1; index <= count; index++){
    for(auto item : filters){
        if(item.first.get_type() != sol::type::string){
            MP_ERROR("Check filter config failed: 'filters' key not all string");
            ret = -1;
            continue;//break;
        }

        if(item.second.get_type() != sol::type::table){
            MP_ERROR("Check filter config failed: 'filters' value not all tables");
            ret = -1;
            continue;//break;
        }
        std::string id = item.first.as<std::string>();
        sol::table  filter = item.second.as<sol::table>();
        //add a id value for FilterBase::init get it
        filter["id"] = id;
        create_filter(id,filter);
    }
    return ret;
}

int32_t GraphImplement::check_graph_connectings()
{
    filters_flow_.create_from_graph(this);
    return 0;
}

int32_t GraphImplement::do_flow_command(const std::string &command, const NativeValue &param)
{
    const auto& ret = filters_flow_.process_command(command, param);
    if(ret == 0) {
        auto status = command_cause_status(command);
        switch_status(status);
    } else {
        MP_ERROR("GraphImplement::do_flow_command failed, forcing anything to stop... ");
        filters_flow_.process_command(kGraphCommandStop, param);
        switch_status(kStatusStoped);
    }
    return ret;
}

int32_t GraphImplement::switch_status(GraphStatus status)
{
    MP_LOG_DEAULT("==== GraphImplement::switch_status {}", status);
    if(status_ != status){
        status_ = status;
        lua_status_function_(graph_context_,status);
    }
    return 0;
}

table GraphImplement::make_features_table()
{
    features_ = Factory::factory()->features();
    sol::state& vm = graph_vm_;
    auto feature_table = vm.create_table();
#if defined(HAS_ROCKCHIP_MPP)
    features_["rockchip_mpp"] = 1;
#endif
    for(auto feature : features_){
        feature_table[feature.first] = feature.second;
    }
    return feature_table;
}


int32_t GraphImplement::do_connect(IFilter *sender, IFilter *receiver, int32_t sender_pin_index, int32_t receiver_pin_index)
{
    auto ret = receiver->connect_before_match(sender);
    if(ret < 0)
        return ret;
    auto s = sender->id();
    auto r = receiver->id();
    MP_LOG_DEAULT("Connect Filter {}[{}] -> {}[{}]:",s.c_str(),sender_pin_index,r.c_str(),receiver_pin_index);

    PinVector sender_pins;
    PinVector receiver_pins;

    sender_pin_index -= 1;//lua use 1 for start
    if(sender_pin_index>= 0){
        auto pin = sender->get_pin(kOutputPin,sender_pin_index);
        if(pin)
            sender_pins.push_back(pin);
    }

    receiver_pin_index -= 1;//lua use 1 for start
    if(sender_pin_index>= 0){
        auto pin = receiver->get_pin(kInputPin,sender_pin_index);
        if(pin && pin->sender() == nullptr)
            receiver_pins.push_back(pin);
    }

    if(sender_pin_index < 0){
        int32_t index = 0;
        PinVector connected_pins;
        for(;;){
            auto pin = sender->get_pin(kOutputPin,index++);
            if(pin==nullptr)
                break;
            if(pin->receivers().size()){
                //cache already connected pins, (lower priority)
                connected_pins.push_back(pin);
            }
            else{
                sender_pins.push_back(pin);
            }
        }

        //put already connected pins to tail
        for(auto pin : connected_pins)
            sender_pins.push_back(pin);
    }

    if(receiver_pin_index < 0){
        int32_t index = 0;
        for(;;){
            auto pin = receiver->get_pin(kInputPin,index++);
            if(pin==nullptr)
                break;
            if(pin->sender() == nullptr){
                //a receiver pin connect to must without sender
                receiver_pins.push_back(pin);
            }
        }
    }

    if(sender_pins.empty()){
        MP_ERROR("connect filter failed,sender not has valid pins.");
        return emit_error("graph",kErrorConnectFailedNoOutputPin);
    }
    if(receiver_pins.empty()){
        MP_ERROR("connect filter failed,receiver_pins not has valid pins.");
        return emit_error("graph",kErrorConnectFailedNoInputPin);
    }

    int32_t connected_count = 0;
    for(auto sender_pin : sender_pins){
        for(auto receive_pin : receiver_pins){
            if(receive_pin->sender() != nullptr)
                continue;
            auto& sender_formats = sender_pin->formats();
            auto& receiver_formats = receive_pin->formats();
            auto ret = sender->connect_constraint_output_format(sender_pin.Get(),receiver_formats);
            if(ret < 0)
                continue;

            int format_index = receiver->connect_match_input_format(sender_pin.Get(),receive_pin.Get());
            if(format_index >= 0){
                sender->connect_chose_output_format(sender_pin.Get(),format_index);
                sender->connect(sender_pin.Get(),receive_pin.Get());
                auto str_format = sdp::FormatUtils::to_printable(sender_formats[format_index]);
                MP_LOG_DEAULT("SUCCESSED: connect filter done: [{}].",str_format.c_str());
                connected_count++;
                //receiver connected, match next sender pin
                break;
            }
        }
    }

    if(connected_count > 0)
        return 0;

    MP_ERROR("connect filter failed,sender/receiver pins format not march.");
    return emit_error("graph",kErrorConnectFailedNotMatchFormat);
}

int32_t GraphImplement::do_disconnet_sender(IFilter *receiver, int32_t receiver_pin_index)
{
    return 0;
}

int32_t GraphImplement::do_disconnet_receiver(IFilter *receiver, int32_t receiver_pin_index)
{
    return 0;
}

int32_t GraphImplement::do_disconnet_all()
{
    for(auto& filter : filters_){
        FilterHelper::disconnect_output(filter.second.Get());
    }
    return 0;
}


const std::map<std::string ,FilterPointer> &GraphImplement::filters()
{
    return filters_;
}

int32_t GraphImplement::create_filter(const std::string &id,const  sol::table &filter)
{
    MP_LOG_DEAULT("-------- GraphImplement::create_filter: {} ", id.data());
    std::string module = filter.get_or("module",std::string());
    if(module.empty() || id.empty() || filters_.find(id) != filters_.end()){
        MP_ERROR("Check filter [id:{}][module:{}] failed: property unset",id,module);
        return kErrorInvalidParameter;
    }
    auto filter_object = Factory::factory()->create_filter(module);
    if(!filter_object){
        MP_ERROR("Create filter [id:{}][module:{}] failed: no registered module",id,module);
        return kErrorInvalidParameter;
    }
    filters_[id] = filter_object;
    int32_t ret = filter_object->initialize(this,filter);
    if(ret < 0){
        MP_ERROR("Init filter [id:{}][module:{}] failed:{}",id,module,ret);
        remove_filter(id);
        return kErrorInvalidParameter;
    }
    return 0;
}

int32_t GraphImplement::remove_filter(const std::string &id)
{
    if(status_ == kStatusRunning)
        return kErrorStatusNotAcceped;
    auto it = filters_.find(id);
    if(it!= filters_.end()){
        FilterHelper::disconnect_output(it->second.Get());
        it->second->disconnect_input(-1);
        filters_.erase(id);
        graph_context_["filters"][id] = sol::lua_nil;
    }
    return 0;
}


int32_t GraphImplement::set_property(const std::string &property, NativeValue &value)
{
    if(property == kGraphPropertyCurrentPts){
        //use timeline decision , not need it anymore.
        //current_pts_ = value;
        //MP_LOG_DEAULT(">>>>Graph Refresh PTS:{}",current_pts_);
    }
    else {
        properties_[property]=value;
    }
    return 0;
}

int32_t GraphImplement::get_property(const std::string &property, NativeValue &value)
{
    if(property == kGraphPropertyCurrentPts){
        current_pts_ = filters_flow_.timeline();
        value = (int)current_pts_;
    }
    else{
        auto it = properties_.find(property);
        if(it == properties_.end()){
            value = it->second;
        }
        else
            return -1;
    }

    return 0;
}

int32_t GraphHelper::append_video_observer(sdp::IGraph *graph, const std::string& video_output_filter, IFilterExtentionVideoOutputProxy::Observer* observer, bool append_remove, ComPointer<IFilterExtentionVideoOutputProxy>& filter_used){
    filter_used = nullptr;
    if(graph == nullptr || observer == nullptr)
        return -1;

    FilterPointer render;
    auto& filters = graph->filters();
    for(auto& item : filters){
        if(item.second->type() != kFilterTypeVideoOutput)
            continue;
        if(video_output_filter.size()){
            if(item.second->id() == video_output_filter){
                render = item.second;
                break;
            }
            continue;
        }
        render = item.second;
        break;
    }

    if(!render)
        return kErrorFilterInvalid;

    filter_used = render.As<IFilterExtentionVideoOutputProxy>();
    if(!filter_used)
        return kErrorFilterNoInterfce;

    if(append_remove)
        filter_used->append_observer(observer);
    else
        filter_used->remove_observer(observer);

    return 0;
}

FilterPointer GraphHelper::get_filter(IGraph *graph, const std::string &filter_id)
{
    if(graph == nullptr)
        return nullptr;

    FilterPointer render;
    auto& filters = graph->filters();
    auto it = filters.find(filter_id);
    if(it == filters.end())
        return nullptr;
    return it->second;
}

int32_t  GraphHelper::get_filter_property(IGraph* graph, const std::string& filter_id, const std::string& property, sol::NativeValue &value){
    FilterPointer filter = get_filter(graph,filter_id);
    if(!filter)
        return -1;
    return filter->get_property(property,value);
}

int32_t GraphHelper::set_filter_property(IGraph *graph, const std::string &filter_id, const std::string &property, NativeValue &value)
{
    FilterPointer filter = get_filter(graph,filter_id);
    if(!filter)
        return -1;
    return filter->set_property(property,value);
}

}
