#ifndef FILTERGENERAL_H
#define FILTERGENERAL_H

#include "core_includes.h"

namespace mr::sdmp {

class GeneralFilter : public IFilter
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
    //////init
    COM_IMP_METHOD( initialize(IGraph* graph,const Value & config_value) )

    //////basic info
    COM_IMP_METHOD_RET( const std::string&, id() )
    COM_IMP_METHOD_RET( FilterType, type() )
    COM_IMP_METHOD( level() )
    COM_IMP_METHOD( set_level(int32_t level) )
    COM_IMP_METHOD( activable() )

    //////pins operator
    COM_IMP_METHOD( add_pin(PinPointer pin) )
    COM_IMP_METHOD( remove_pin(PinDirection direction,int32_t index) )//remove a pin, <0 for clear
    COM_IMP_METHOD_RET( PinPointer, get_pin(PinDirection direction, int32_t index) )
    COM_IMP_METHOD_RET( PinVector&, get_pins(PinDirection direction) )

    //////property and methods call
    COM_IMP_METHOD( get_property(const std::string& property,Value& value) )
    COM_IMP_METHOD( set_property(const std::string& property, const Value& value, bool from_script = true) )
    COM_IMP_METHOD_RET( Value, call_method(const std::string& method, const Value& param) )

    //////pins connecting operators
    COM_IMP_METHOD( connect_constraint_output_format(IPin* output_pin, const std::vector<Format> &format) )
    COM_IMP_METHOD( connect_before_match(IFilter *sender_filter) )
    COM_IMP_METHOD( connect(IPin* output_pin,IPin* input_pin) )
    /*basic disconnect output pin method #disconnect_output(int32_t output_pin) #disconnect_output() use this implement*/
    COM_IMP_METHOD( disconnect_output(int32_t output_pin,IPin* input_pin) )
    /*disconnect a input pin's  sender, -1 for disconnect all*/
    COM_IMP_METHOD( disconnect_input(int32_t input_pin) )

    //running loop
    COM_IMP_METHOD( master_loop(bool before_after) )
    COM_IMP_METHOD( process_command(const std::string& command,const Value& param) )

    // method must implement by final filters
    // virtual int32_t connect_match_input_format(IPin *sender_pin,IPin *input_pin) = 0;
    // virtual int32_t connect_chose_output_format(IPin* output_pin, int32_t index) = 0;
    // virtual int32_t receive(IPin* input_pin,FramePointer frame) = 0;
    // virtual int32_t requare(int32_t duration,const std::vector<PinIndex>& output_pins) = 0;

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
    IGraph*         graph_ = nullptr;
    sol::table      filter_state_;
    std::string     id_;
    std::string     module_;

    FilterDelear    declears_;
    std::map<std::string,Value> properties_;

    std::mutex  async_mutex_;
    std::vector<PropertyAsyncRequest> properties_async_request_;
    //a direct member of status for simple access
    GraphStatus status_ = kStatusNone;

    //activable
    bool            activable_ = true;
    //pin level,from render=0 source=n
    int32_t         level_     = 0;
private:
    PinVector input_pins_;
    PinVector output_pins_;
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
class GeneralFilterTypedAs : public GeneralFilter{
public:

    GeneralFilterTypedAs()
        :GeneralFilter(__t_uuidof(FilterImplement)){
    }
    ~GeneralFilterTypedAs(){}
};

}

#endif // FILTERGENERAL_H
