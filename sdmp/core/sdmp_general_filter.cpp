#include "sdmp_general_filter.h"
#include "sdmp_general_pin.h"

#include <sstream>
namespace mr::sdmp {


int32_t GeneralFilter::initialize(IGraph *graph, const Value &filter_values)
{
    assert(filter_values.type_ == kPorpertyLuaTable);

    auto config = filter_values.as<sol::table>();

    id_ = config["id"].get_or(std::string(""));
    module_ = config["module"].get_or(std::string(""));

    graph_ = graph;

    if(graph_){
        //not all filter is manage/used by graph, it used outside (such as audio filter in engine)
        sol::state& vm = *graph_->vm();
        sol::optional<sol::table> graph_state = SDMP_GRAPH_GET_CONTEXT(graph_);
        if(graph_state == sol::nullopt){
            //special filter not has lua global variant 'graph','graph.filters','graph.filters[id]'
            filter_state_ = config;
        }
        else{
            //normal graph must have 'graph.filters' as filters list, and 'graph.filters[id]' as filter's params
            //when create filter from lua,it's same as initialize 2nd parameter 'filter_values'
            sol::optional<sol::table> context_opt = graph_state.value()["filters"][id_];
            if(context_opt == sol::nullopt)
            {
                MR_ERROR("Filter bind to script failed: graph.filters.{} not fount",id_.c_str());
                return kErrorBadScript;
            }
            filter_state_ = context_opt.value();
        }

        filter_state_["context"] = static_cast<IFilter*>(this);
        //make a global lua value same as fileter-id
        vm[id_] = filter_state_;
    }

    sync_filter_to_script();
    sync_pins_to_script(kInputPin);
    sync_pins_to_script(kOutputPin);
    return 0;
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
            case kPorpertyLuaTable:properties_[item.name_] = Value(sol::table());break;
            case kPorpertyNone:
            continue;
        }
    }

    async_queue_.appendListener(kGeneralFilterAsyncSetPropertyEvent,[](GeneralFilter * filter, const std::string & property, const Value &value){
        filter->set_property(property,value,false);
    });
}

GeneralFilter::~GeneralFilter()
{
    remove_pin(kInputPin,kPinIndexAll);
    remove_pin(kOutputPin,kPinIndexAll);
    MR_INFO("### GeneralFilter Release: {} {} {}",module_,id_,(void*)this);
}

const std::string& GeneralFilter::id()
{
    return id_;
}

FilterType GeneralFilter::type()
{
    return declears_.type;
}


int32_t GeneralFilter::activable()
{
    return activable_;
}

int32_t GeneralFilter::level()
{
    return level_;
}

int32_t GeneralFilter::set_level(int32_t level)
{
    level_ = level;
    return level;
}

int32_t GeneralFilter::add_pin(PinPointer pin)
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

int32_t GeneralFilter::remove_pin(PinDirection direction, int32_t index)
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

PinPointer GeneralFilter::get_pin(PinDirection direction, int32_t index)
{
    auto& pins = (direction == kInputPin) ? input_pins_ : output_pins_;
    if(size_t(index) >= pins.size())
        return PinPointer();
    return pins[size_t(index)];
}

PinVector &GeneralFilter::get_pins(PinDirection direction)
{
    if(direction == kInputPin)
        return input_pins_;
    else
        return output_pins_;
}

Value GeneralFilter::call_method(const Arguments& args)
{
    MR_WARN("the filter of {} call_method not implement!",module_);
    (void)args;
    return 0;
}

int32_t GeneralFilter::process_command(const std::string &command, const Value& param)
{
    auto status = command_cause_status(command);
    if(status != kStatusNone){
        switch_status(status);
    }
    if(command == kFilterCommandActive)
        activable_ = true;
    else if(command == kFilterCommandInactive)
        activable_ = false;
    return 0;
}

int32_t GeneralFilter::connect_constraint_output_format(IPin *, const std::vector<Format> &)
{
    return 0;
}

int32_t GeneralFilter::connect_before_match(IFilter *sender_filter)
{
    (void)sender_filter;
    return 0;
}

int32_t GeneralFilter::connect(IPin *output_pin, IPin *input_pin)
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

int32_t GeneralFilter::disconnect_output(int32_t output_pin, IPin *input_pin)
{
    auto pin = get_pin(kOutputPin,output_pin);
    if(pin == nullptr)
        return kErrorInvalidParameter;
    input_pin->disconnect(pin.Get());
    pin->disconnect(input_pin);
    return 0;
}

int32_t GeneralFilter::disconnect_input(int32_t input_pin)
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
        status_ = (GraphStatus)value.as_int64();
    }

    auto it = properties_.find(property);
    if(it == properties_.end()){
        MR_ERROR("Filter {} not has a property {}",id_.c_str(),property.c_str());
        return 0;
    }
    auto& property_value = it->second;
    if(property_value.type_ != value.type_){
        MR_ERROR("Filter {} property {} type not match",id_.c_str(),property.c_str());
        return -1;
    }
    else if(property_value.readonly_ && from_script){
        MR_ERROR("Filter {} property {} is read only,script not allowed overwrite",id_.c_str(),property.c_str());
        return -2;
    }
    else{
        property_value = value;
        if(!from_script)
            sync_set_property_to_script(property,value);

        MR_LOG_DEAULT("Filter {} set property {} to {}",id_.c_str(),property.c_str(),StringUtils::printable(property_value));
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
    async_queue_.process();

    //direct changed property use properties_["xxx"]=xxx, sync to script
    for(auto& item : properties_){
        if(item.second.value_changed_){
            sync_set_property_to_script(item.first,item.second);
            item.second.value_changed_ = false;
        }
    }
    return 0;
}

int32_t GeneralFilter::property_changed(const std::string &property, Value &value)
{    
    return 0;
}

int32_t GeneralFilter::async_set_property_to_stript(const std::string &property, const Value &value, const Value &call_param)
{
    std::lock_guard<std::mutex> lock(async_mutex_);
    async_queue_.enqueue(kGeneralFilterAsyncSetPropertyEvent,this,property,value);
    return 0;
}

int32_t GeneralFilter::sync_set_property_to_script(const std::string &property,const Value &value)
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

int32_t GeneralFilter::sync_filter_to_script()
{
    for(auto& item : properties_){
        std::string property = item.first;
        Value& symbol = item.second;
        auto propert_opt = filter_state_[property];
        if(propert_opt.valid()){
            sol::lua_value value = filter_state_[property];
            symbol.from_lua_value(&value);
            MR_LOG_DEAULT("Filter {} read pre-defined property {} to {}",id_.c_str(),property.c_str(),StringUtils::printable(symbol));
        }
        else{
            sync_set_property_to_script(property,symbol);
            MR_LOG_DEAULT("Filter {} create un-defined property {} to {}",id_.c_str(),property.c_str(),StringUtils::printable(symbol));
        }
    }
    return 0;
}

int32_t GeneralFilter::sync_pins_to_script(PinDirection direction)
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

int32_t GeneralFilter::sync_update_pin_format(PinDirection direction, int32_t pin_index, int32_t format_index, const Format &format)
{
    auto pin = get_pin(direction,pin_index);
    if(pin == nullptr)
        return kErrorInvalidParameter;

    if(format_index >= pin->formats().size())
        return -2;

    pin->formats()[format_index] = format;
    sync_pins_to_script(direction);
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
        async_set_property_to_stript(kFilterPropertyStatus,value);
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

