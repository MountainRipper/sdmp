#ifndef FILTERGENERAL_H
#define FILTERGENERAL_H

#include "core_includes.h"

namespace mr::sdmp {

class GeneralFilter : public GeneralFilterBase
{
public:
    struct PropertyAsyncRequest{
        std::string prop;
        Value value;
        Value call_param;
    };

    GeneralFilter(const TGUID & filter_clsid);
    ~GeneralFilter();

    // IFilter interface
public:
    //virtual function implements of GeneralFilterBase
    virtual FilterType type();
    virtual int32_t initialize(IGraph *graph, const sol::table &config);
    virtual int32_t process_command(const std::string& command,const Value& param);
    virtual int32_t get_property(const std::string& property,Value& value);
    virtual int32_t set_property(const std::string& property, const Value& value, bool from_script = false);
    virtual int32_t master_loop(bool before_after);

    //virtual function declear by GeneralFilter
    virtual int32_t property_changed(const std::string& property, Value& symbol);
protected:
    int32_t set_property_async(const std::string& property, const Value& value,const Value& call_param = 0);
    int32_t put_property_to_script(const std::string& property, const Value &value);

    int32_t bind_filter_to_script();
    int32_t bind_pins_to_script(PinDirection direction);
    int32_t update_pin_format(PinDirection direction, int32_t pin_index,int32_t format_index, const Format& format);    

    PinPointer create_general_pin(AVMediaType type,PinDirection direction);
    PinPointer create_general_pin(PinDirection direction,const Format& format);
    PinPointer create_general_pin(PinDirection direction, const std::vector<Format>& formats);

    int32_t switch_status(GraphStatus status);
    //-1 for all output pin
    int32_t deliver_eos_frame(int32_t pin_index = -1);
protected:
    FilterDelear declears_;
    SolPropertiesMap properties_;

    std::mutex  async_mutex_;
    std::vector<PropertyAsyncRequest> properties_async_request_;
    //a direct member of status for simple access
    GraphStatus status_ = kStatusNone;
};

template<class PinClass>
class FilterPinFactory{
public:
    static PinPointer filter_create_pin(FilterPointer filter, PinDirection direction, const std::vector<Format>& formats){
        ComPointer<IPin> pin;
        pin.CoCreateInstanceDirectly<PinClass>();
        pin->initialize(filter,formats,direction);
        filter->add_pin(pin);
        return pin;
    }
    static PinPointer filter_create_pin(FilterPointer filter, PinDirection direction, const Format& format){
        std::vector<Format> formats;
        formats.push_back(format);
        ComPointer<IPin> pin;
        pin.CoCreateInstanceDirectly<PinClass>();
        pin->initialize(filter,formats,direction);
        filter->add_pin(pin);
        return pin;
    }
    static PinPointer filter_create_pin(FilterPointer filter, AVMediaType type,PinDirection direction){
        Format format;
        format.type = type;
        std::vector<Format> formats = {format};
        ComPointer<IPin> pin;
        pin.CoCreateInstanceDirectly<PinClass>();
        pin->initialize(filter,formats,direction);
        filter->add_pin(pin);
        return pin;
    }
};

template<class FilterImplement>
class GeneralFilterObjectRoot : public GeneralFilter{
public:

    GeneralFilterObjectRoot()
        :GeneralFilter(__t_uuidof(FilterImplement)){
    }
    ~GeneralFilterObjectRoot(){}
};

}

#endif // FILTERGENERAL_H
