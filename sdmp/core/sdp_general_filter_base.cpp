#include <logger.h>
#include <sol_lua_operator.h>
#include "sdp_general_filter_base.h"
#include "sdpi_graph.h"
#include "sdpi_pin.h"
namespace sdp {

FilterType GeneralFilterBase::type()
{
    return kFilterTypeNone;
}

int32_t GeneralFilterBase::activable()
{
    return activable_;
}

int32_t GeneralFilterBase::initialize(IGraph* graph,const sol::table & config)
{
    id_ = config["id"].get_or(std::string(""));
    module_ = config["module"].get_or(std::string(""));

    graph_ = graph;

    if(graph_){
        //not all filter is manage/used bu graph, it used outside (such as audio filter in engine)
        sol::state& vm = *graph_->vm();
        sol::table& graph_state = graph_->graph();
        sol::optional<sol::table> context_opt = graph_state["filters"][id_];
        if(context_opt == sol::nullopt)
        {
            MP_ERROR("Filter bind to script failed: graph.filters.{} not fount",id_.c_str());
            return kErrorBadScript;
        }
        filter_state_ = context_opt.value();
        vm[id_] = filter_state_;

        filter_state_["context"] = static_cast<IFilter*>(this);
        filter_state_["setProperty"] = &IFilter::set_property_lua;
        filter_state_["callMethod"] = &IFilter::call_method_lua;
    }

    return (!id_.empty() && !module_.empty()) ? 0 : -1;
}

PinPointer GeneralFilterBase::get_pin(PinDirection direction, int32_t index)
{
    auto& pins = (direction == kInputPin) ? input_pins_ : output_pins_;
    if(size_t(index) >= pins.size())
        return PinPointer();
    return pins[size_t(index)];
}

PinVector &GeneralFilterBase::get_pins(PinDirection direction)
{
    if(direction == kInputPin)
        return input_pins_;
    else
        return output_pins_;
}

int32_t GeneralFilterBase::get_property(const std::string& property, NativeValue &value){
    (void)property;
    (void)value;
    return 0;
}

int32_t GeneralFilterBase::set_property(const std::string &property, const NativeValue &value, bool from_script)
{
    (void)property;
    (void)value;
    (void)from_script;
    return 0;
}

int32_t GeneralFilterBase::set_property_lua(const std::string& property,const sol::lua_value& value){
    NativeValue native_value;
    int32_t ret = native_value.create_from_lua_value(value);
    if(ret < 0){
        MP_ERROR("FilterBase::set_property_lua bad property type,must[double,double[],string,void*-pointer,lua-function] prop:{} type:{}",property.c_str(),(int)value.value().get_type());
        return ret;
    }
    return set_property(property,native_value,true);
}

NativeValue GeneralFilterBase::call_method(const std::string &method, const NativeValue &param)
{
    (void)method;
    (void)param;
    return 0;
}

lua_value GeneralFilterBase::call_method_lua(const std::string &method, const lua_value &param)
{
    NativeValue native_value;
    int32_t ret = native_value.create_from_lua_value(param);
    if(ret < 0){
        MP_ERROR("FilterBase::call_method_lua bad param type,must[double,double[],string,void*-pointer,lua-function] prop:{} type:{}",method.c_str(),(int)param.value().get_type());
        return ret;
    }
    NativeValue return_value = call_method(method,native_value);

    return return_value.create_lua_value(*graph_->vm());
}

int32_t GeneralFilterBase::master_loop(bool before_after)
{
    (void)before_after;
    return 0;
}

int32_t GeneralFilterBase::process_command(const std::string &command, const NativeValue &param)
{
    if(command == kFilterCommandActive)
        activable_ = true;
    else if(command == kFilterCommandInactive)
        activable_ = false;

    return 0;
}

int32_t GeneralFilterBase::connect_constraint_output_format(IPin *, const std::vector<Format> &)
{
    return 0;
}

int32_t GeneralFilterBase::connect_before_match(IFilter *sender_filter)
{
    (void)sender_filter;
    return 0;
}

int32_t GeneralFilterBase::connect(IPin *output_pin, IPin *input_pin)
{
    bool found_out = false;
    for(auto in : output_pins_){
        if(in.Get() == output_pin){
            found_out = true;
            break;
        }
    }

    if(!found_out)
        return kErrorConnectFailedNoInputPin;

    auto ret = 0;
    ret = output_pin->connect(input_pin);
    if(ret < 0)
        return ret;
    ret =  input_pin->connect(output_pin);
    return ret;
}

int32_t GeneralFilterBase::disconnect_output(int32_t output_pin, IPin *input_pin)
{
    auto pin = get_pin(kOutputPin,output_pin);
    if(pin == nullptr)
        return kErrorInvalidParameter;
    input_pin->disconnect(pin.Get());
    pin->disconnect(input_pin);
    return 0;
}

int32_t GeneralFilterBase::disconnect_input(int32_t input_pin)
{
    PinVector pins;
    if(input_pin < 0)
        pins = input_pins_;
    else{
        if(input_pin >= input_pins_.size())
            return kErrorInvalidParameter;
        pins.push_back(input_pins_[input_pin]);
    }

    for(auto pin : pins){
        auto sender = pin->sender();
        if(sender){
            sender->disconnect(pin.Get());
            pin->disconnect(sender);
        }
    }
    return 0;
}

int32_t GeneralFilterBase::FinalRelease()
{
    remove_pin(kInputPin,kPinIndexAll);
    remove_pin(kOutputPin,kPinIndexAll);
    return 0;
}

const std::string& GeneralFilterBase::id()
{
    return id_;
}

int32_t GeneralFilterBase::level()
{
    return level_;
}

int32_t GeneralFilterBase::set_level(int32_t level)
{
    level_ = level;
    return level;
}

int32_t GeneralFilterBase::add_pin(PinPointer pin)
{
    if(pin->direction() == kInputPin){
        pin->set_index(input_pins_.size());
        input_pins_.push_back(pin);
    }
    else if(pin->direction() == kOutputPin){
        pin->set_index(output_pins_.size());
        output_pins_.push_back(pin);
    }
    return 0;
}

int32_t GeneralFilterBase::remove_pin(PinDirection direction, int32_t index)
{
    auto& pins = (direction == kInputPin) ? input_pins_ : output_pins_;
    if(index == kPinIndexAll){
        for(auto& pin : pins){
            pin->uninitialize();
        }
        pins.clear();
        return 0;
    }

    if(index >= pins.size() || index < 0)
        return kErrorInvalidParameter;

    pins[index]->uninitialize();
    pins.erase(pins.begin()+index);

    //refresh index
    index = 0;
    for(auto pin : input_pins_){
        pin->set_index(index++);
    }
    return 0;
}


}
