#include "sdp_general_filter.h"
#include "sdp_general_pin.h"

#include <sstream>
namespace mr::sdmp {


int32_t GeneralFilter::initialize(IGraph *graph, const sol::table &config)
{
    if(GeneralFilterBase::initialize(graph,config) < 0)
        return -1;
    bind_filter_to_script();
    bind_pins_to_script(kInputPin);
    bind_pins_to_script(kOutputPin);
    return 0;
}
FilterType GeneralFilter::type()
{
    return declears_.type;
}

GeneralFilter::GeneralFilter(const TGUID & filter_clsid)
{
    const FilterDelear* declear = Factory::filter_declear(filter_clsid);
    //declears_ =
    assert(declear != nullptr);
    declears_ = *declear;

    properties_[kFilterPropertyStatus]= Value((double)kStatusStoped);
    for(auto &item : declears_.properties){
        switch (item.type_) {
            case kPorpertyNumber: properties_[item.name_] = Value(std::any_cast<double>(item.value_));break;
            case kPorpertyString: properties_[item.name_] = Value(std::any_cast<std::string>(item.value_));break;
            case kPorpertyBool: properties_[item.name_] = Value(std::any_cast<bool>(item.value_));break;
            case kPorpertyNumberArray: properties_[item.name_] = Value(std::any_cast<std::vector<double>>(item.value_));break;
            case kPorpertyStringArray: properties_[item.name_] = Value(std::any_cast<std::vector<std::string>>(item.value_));break;
            case kPorpertyPointer: properties_[item.name_] = Value(std::any_cast<void*>(item.value_));break;
            case kPorpertyLuaFunction: properties_[item.name_] = Value(sol::function());break;
            case kPorpertyNone:
                continue;
        }
    }
}

GeneralFilter::~GeneralFilter()
{
    MP_INFO("### GeneralFilter Release: {} {} {}",module_,id_,(void*)this);
}

int32_t GeneralFilter::process_command(const std::string &command, const Value& param)
{
    auto status = command_cause_status(command);
    if(status != kStatusNone){
        switch_status(status);
    }
    return GeneralFilterBase::process_command(command,param);
}

int32_t GeneralFilter::get_property(const std::string &property, Value &value)
{
    if(property == kFilterPropertyStatus){
        value = (int64_t)status_;
        return 0;
    }

    auto it = properties_.find(property);
    if(it != properties_.end()){
        value = it->second;
        return 0;
    }
    value = .0;
    return -1;
}

int32_t GeneralFilter::set_property(const std::string &property, const Value &value, bool from_script)
{
    if(property == kFilterPropertyStatus){
        int64_t v = value;
        status_ = (GraphStatus)v;
    }

    auto it = properties_.find(property);
    if(it == properties_.end()){
        MP_ERROR("Filter {} not has a property {}",id_.c_str(),property.c_str());
        return 0;
    }
    auto& property_value = it->second;
    if(property_value.type_ != value.type_){
        MP_ERROR("Filter {} property {} type not match",id_.c_str(),property.c_str());
        return -1;
    }
    else if(property_value.readonly_ && from_script){
        MP_ERROR("Filter {} property {} is read only,script not allowed overwrite",id_.c_str(),property.c_str());
        return -2;
    }
    else{
        property_value = value;
        if(!from_script)
            put_property_to_script(property,value);

        MP_LOG_DEAULT("Filter {} set property {} to {}",id_.c_str(),property.c_str(),StringUtils::printable(property_value));
        property_changed(property,property_value);
        //already changed
        property_value.value_changed_ = false;
        return 0;
    }
    return 0;
}

int32_t GeneralFilter::master_loop(bool before_after)
{
    std::lock_guard<std::mutex> lock(async_mutex_);
    //put async properties, from set_property_async()
    for(auto& item : properties_async_request_) {
        if(item.prop == kFilterPropertyStatus) {
            status_ = GraphStatus((int64_t)item.value);
        }
        set_property(item.prop,item.value);
    }
    properties_async_request_.clear();

    //direct changed property use properties_["xxx"]=xxx, sync to script
    for(auto& item : properties_){
        if(item.second.value_changed_){
            put_property_to_script(item.first,item.second);
            item.second.value_changed_ = false;
        }
    }
    return 0;
}

int32_t GeneralFilter::property_changed(const std::string &property, Value &value)
{    
    return 0;
}

int32_t GeneralFilter::set_property_async(const std::string &property, const Value &value, const Value &call_param)
{
    std::lock_guard<std::mutex> lock(async_mutex_);
    PropertyAsyncRequest request = {property,value,call_param};
    properties_async_request_.push_back(request);
    return 0;
}

int32_t GeneralFilter::put_property_to_script(const std::string &property,const Value &value)
{
    if(value.type_ == kPorpertyNumber)
        filter_state_[property] = value.as_double();
    if(value.type_ == kPorpertyBool)
        filter_state_[property] = value.as_bool();
    if(value.type_ == kPorpertyString)
        filter_state_[property] = value.as_string();
    if(value.type_ == kPorpertyNumberArray)
        filter_state_[property] = value.as_double_vector();
    if(value.type_ == kPorpertyStringArray)
        filter_state_[property] = value.as_string_vector();
    if(value.type_ == kPorpertyPointer)
        filter_state_[property] = value.as_pointer();
    if(value.type_ == kPorpertyLuaFunction)
        filter_state_[property] = value.as<sol::function>();
    if(value.type_ == kPorpertyLuaTable)
        filter_state_[property] = value.as<sol::table>();
    return 0;
}

int32_t GeneralFilter::bind_filter_to_script()
{
//    sol::state& vm = graph_state_;
//    sol::table& graph = graph_->graph();
//    sol::optional<sol::table> context_opt = graph["filters"][id_];
//    if(context_opt == sol::nullopt)
//    {
//        lMP_ERROR("Filter bind to script failed: graph.filters.{} not fount",id_.c_str());
//    }
//    filter_state_ = context_opt.value();
//    vm[id_] = filter_state_;
//    filter_state_["context"] = static_cast<FilterBase*>(this);
//    filter_state_["setProperty"] = &FilterBase::set_property_lua;

    for(auto& item : properties_){
        std::string property = item.first;
        Value& symbol = item.second;
        auto propert_opt = filter_state_[property];
        if(propert_opt.valid()){
            sol::lua_value value = filter_state_[property];
            symbol = Value(&value);
            MP_LOG_DEAULT("Filter {} read pre-defined property {} to {}",id_.c_str(),property.c_str(),StringUtils::printable(symbol));
        }
        else{
            put_property_to_script(property,symbol);
            MP_LOG_DEAULT("Filter {} create un-defined property {} to {}",id_.c_str(),property.c_str(),StringUtils::printable(symbol));
        }
    }
    return 0;
}

int32_t GeneralFilter::bind_pins_to_script(PinDirection direction)
{
    std::string key = (direction == kInputPin ? "pinsInput" : "pinsOutput");
    auto& pins = get_pins(direction);
    sol::state& vm = *graph_->vm();
    sol::table pins_table = vm.create_table();
    int pin_index = 1;
    for(auto pin : pins){
        sol::table formats_table = vm.create_table();
        std::string type_name;
        const std::vector<Format> &formats = pin->formats();
        int format_index = 1;
        for(auto& format : formats){
            sol::table format_table = vm.create_table();
            sdmp::FormatUtils::to_lua_table(format,format_table);
            formats_table[format_index++] = format_table;
            type_name = format_table.get_or("type_name",std::string());
        }
        pins_table[pin_index++] =  vm.create_table_with("index",pin->index(),
                                                        "formats",formats_table,
                                                        "type",type_name,
                                                        "context",pin.Get(),
                                                        "setActive",&IPin::active);
    }
    filter_state_[key] = pins_table;
    return 0;
}

int32_t GeneralFilter::update_pin_format(PinDirection direction, int32_t pin_index, int32_t format_index, const Format &format)
{
    auto pin = get_pin(direction,pin_index);
    if(pin == nullptr)
        return kErrorInvalidParameter;

    if(format_index >= pin->formats().size())
        return -2;

    pin->formats()[format_index] = format;
    bind_pins_to_script(direction);
    return 0;
}

PinPointer GeneralFilter::create_general_pin(AVMediaType type, PinDirection direction)
{
    FilterPointer filter(static_cast<IFilter*>(this));
    return FilterPinFactory<GeneralPinObject>::filter_create_pin(filter,type,direction);
}

PinPointer GeneralFilter::create_general_pin(PinDirection direction,const Format& format)
{
    FilterPointer filter(static_cast<IFilter*>(this));
    return FilterPinFactory<GeneralPinObject>::filter_create_pin(filter,direction,format);
}

PinPointer GeneralFilter::create_general_pin(PinDirection direction,const std::vector<Format> &formats)
{
    FilterPointer filter(static_cast<IFilter*>(this));
    return FilterPinFactory<GeneralPinObject>::filter_create_pin(filter,direction,formats);
}

int32_t GeneralFilter::switch_status(GraphStatus status)
{
    if(status == status_)
        return 0;
    Value value((double)status);
    if(graph_->in_master_loop())
        set_property(kFilterPropertyStatus,value);
    else
        set_property_async(kFilterPropertyStatus,value);
    status_ = status;
    return 0;
}

int32_t GeneralFilter::deliver_eos_frame(int32_t pin_index)
{
    auto pin = get_pin(kOutputPin, pin_index);
    auto eos_frame = FramePointer(new Frame());
    eos_frame->flag = kFrameFlagEos;
    if(pin != nullptr){
        pin->deliver(eos_frame);
    }
    else{
        auto& pins = get_pins(kOutputPin);
        for(auto pin : pins)
            pin->deliver(eos_frame);
    }
    return 0;
}


}

