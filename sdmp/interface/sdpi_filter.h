#ifndef SDPI_FILTER_H_
#define SDPI_FILTER_H_
#include "sdpi_basic_types.h"
#include "sdpi_pin.h"
#include <any>
#include <sol_lua_operator.h>//TODO: remove this use a own-var class
namespace sdp {

//declear for filter create,factory need
typedef FilterPointer (*ObjectCreateFunction)();

enum PropertyType: int32_t{
    kPorpertyNone        = 0,
    kPorpertyNumber      = 1,
    kPorpertyString      = 2,
    kPorpertyBool        = 4,
    kPorpertyNumberArray = 8,
    kPorpertyStringArray = 16,
    kPorpertyPointer     = 32,
    kPorpertyLuaFunction = 64,
    kPorpertyLuaTable    = 128
};
struct FilterProperty{
    PropertyType type;
    std::string name;
    std::any value;
};

struct FilterDelear{
    TGUID       clsid;
    std::string module;
    std::string describe;
    FilterType  type = kFilterTypeNone;
    std::vector<FilterProperty>   properties;
};


COM_INTERFACE("db2baffc-a7ee-11eb-8b91-d7dc2a399837",IFilter)
public:
    COM_METHOD( initialize(IGraph* graph,const sol::table & config) )

    COM_METHOD_RET( const std::string&, id() )
    COM_METHOD_RET( FilterType, type() )
    COM_METHOD( level() )
    COM_METHOD( set_level(int32_t level) )
    COM_METHOD( activable() )

    COM_METHOD( add_pin(PinPointer pin) )
    COM_METHOD( remove_pin(PinDirection direction,int32_t index) )//remove a pin, <0 for clear
    COM_METHOD_RET( PinPointer, get_pin(PinDirection direction, int32_t index) )
    COM_METHOD_RET( PinVector&, get_pins(PinDirection direction) )

    COM_METHOD( get_property(const std::string& property,sol::NativeValue& value) )
    COM_METHOD( set_property(const std::string& property, const sol::NativeValue& value, bool from_script = true) )
    COM_METHOD( set_property_lua(const std::string& property,const sol::lua_value& value) )
    COM_METHOD_RET( sol::NativeValue, call_method(const std::string& method, const sol::NativeValue& param) )
    COM_METHOD_RET( sol::lua_value, call_method_lua(const std::string& method,const sol::lua_value& param) )

    COM_METHOD( master_loop(bool before_after) )
    COM_METHOD( process_command(const std::string& command,const sol::NativeValue& param) )

    COM_METHOD( connect_constraint_output_format(IPin* output_pin, const std::vector<Format> &format) )
    COM_METHOD( connect_before_match(IFilter *sender_filter) )

    COM_METHOD( connect(IPin* output_pin,IPin* input_pin) )
    //basic disconnect output pin method #disconnect_output(int32_t output_pin) #disconnect_output() use this implement
    COM_METHOD( disconnect_output(int32_t output_pin,IPin* input_pin) )
    //disconnect a input pin's  sender, -1 for disconnect all
    COM_METHOD( disconnect_input(int32_t input_pin) )


    COM_METHOD( connect_match_input_format(IPin *sender_pin,IPin *input_pin) )
    COM_METHOD( connect_chose_output_format(IPin* output_pin, int32_t index) )
    COM_METHOD( receive(IPin* input_pin,FramePointer frame) )
    COM_METHOD( requare(int32_t duration,const std::vector<PinIndex>& output_pins) )
};


namespace FilterHelper{

    FilterType type_from_string(const char* type_string);
    //print a filter list
    int32_t print_filter_list(const std::vector<FilterPointer> &list, const std::string& spliter);

    //get in-out filters
    std::set<FilterPointer> get_filter_near(FilterPointer filter,PinDirection direction);
    std::set<FilterPointer> get_filter_senders(FilterPointer filter);
    std::set<FilterPointer> get_filter_receivers(FilterPointer filter);

    //get receivers link from source, used for parse source-output links etc...
    int32_t get_filter_receiver_links(FilterPointer filter, std::vector<std::vector<FilterPointer> > &links);

    //get connected pins info
    int32_t get_filter_linked_pin(FilterPointer sender, FilterPointer receiver, std::vector<PinIndex>& sender_pins, std::vector<PinIndex>& receiver_pins);

    //connect function, use [IFilter::connect]
    int32_t connect(FilterPointer sender,int32_t output_pin_index, FilterPointer receiver, int32_t input_pin_index);
    //disconnect a output pin's all receivers, use [IFilter::disconnect]
    int32_t disconnect_output(FilterPointer filter,int32_t output_pin);
    //disconnect all output pin's all receivers, use [IFilter::disconnect]
    int32_t disconnect_output(FilterPointer filter);

};

//template<class Base>
//class FilterFactory : public Base{
//public:
//    static FilterPointer create_instance(){
//        FilterPointer ptr;
//        ptr.CoCreateInstance(__t_uuidof(Base) );
//        return ptr;
//    }
//
//    static HRESULT create_unknown(IUnknownPointer& unknown){
//        return unknown.CoCreateInstance(__t_uuidof(Base) );
//    }
//
//    static const sdp::FilterDelear& declear(){return declear_;}
//    static sdp::FilterType   types(){return declear_.type;}
//private:
//    static sdp::FilterDelear declear_;
//};
//


}

#endif
