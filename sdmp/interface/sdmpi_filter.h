#ifndef SDPI_FILTER_H_
#define SDPI_FILTER_H_
#include "sdmpi_basic_types.h"
#include "sdmpi_pin.h"
#include <any>
//#include <sol/forward.hpp>//TODO: remove this use a own-var class

namespace mr::sdmp {


//declear for filter create,factory need
typedef FilterPointer (*ObjectCreateFunction)();

struct FilterDelear{
    TGUID       clsid;
    std::string module;
    std::string describe;
    FilterType  type = kFilterTypeNone;
    std::vector<Value>   properties;
};


COM_INTERFACE("db2baffc-a7ee-11eb-8b91-d7dc2a399837",IFilter)
public:
    //////init
    COM_METHOD( initialize(IGraph* graph,const Value & filter_values) )

    //////basic info
    COM_METHOD_RET( const std::string&, id() )
    COM_METHOD_RET( FilterType, type() )
    COM_METHOD( level() )
    COM_METHOD( set_level(int32_t level) )
    COM_METHOD( activable() )

    //////pins operator
    COM_METHOD( add_pin(PinPointer pin) )
    COM_METHOD( remove_pin(PinDirection direction,int32_t index) )//remove a pin, <0 for clear
    COM_METHOD_RET( PinPointer, get_pin(PinDirection direction, int32_t index) )
    COM_METHOD_RET( PinVector&, get_pins(PinDirection direction) )

    //////property and methods call
    COM_METHOD( get_property(const std::string& property,Value& value) )
    COM_METHOD( set_property(const std::string& property, const Value& value, bool from_script = true) )
    COM_METHOD_RET( Value, call_method(const Arguments& args) )

    //////pins connecting operators
    COM_METHOD( connect_constraint_output_format(IPin* output_pin, const std::vector<Format> &format) )
    COM_METHOD( connect_before_match(IFilter *sender_filter) )
    COM_METHOD( connect(IPin* output_pin,IPin* input_pin) )
    /*disconnect output pin's receiver pin*/
    COM_METHOD( disconnect_output(int32_t output_pin,IPin* input_pin) )
    /*disconnect a input pin's  sender, -1 for disconnect all*/
    COM_METHOD( disconnect_input(int32_t input_pin) )

    //running loop
    COM_METHOD( master_loop(bool before_after) )
    COM_METHOD( process_command(const std::string& command,const Value& param) )

    //format match and data stream io,which methods must implement by final filters
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

}

#endif
