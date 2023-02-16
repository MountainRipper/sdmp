#ifndef SDP_FILTER_BASE_H
#define SDP_FILTER_BASE_H
#include <string>
#include <set>
#include <sdpi_filter.h>
#include <sol/sol.hpp>

using namespace sol;
namespace mr::sdmp {
    //base class of filters
    class GeneralFilterBase : public IFilter{
        friend class GeneralPin;
    public:
        virtual ~GeneralFilterBase();

        COM_IMP_METHOD( initialize(IGraph* graph,const sol::table & config) )

        COM_IMP_METHOD_RET( const std::string&, id() )
        COM_IMP_METHOD_RET( FilterType, type() )
        COM_IMP_METHOD( level() )
        COM_IMP_METHOD( set_level(int32_t level) )
        COM_IMP_METHOD( activable() )

        COM_IMP_METHOD( add_pin(PinPointer pin) )
        COM_IMP_METHOD( remove_pin(PinDirection direction,int32_t index) )//remove a pin, <0 for clear
        COM_IMP_METHOD_RET( PinPointer, get_pin(PinDirection direction, int32_t index) )
        COM_IMP_METHOD_RET( PinVector&, get_pins(PinDirection direction) )

        COM_IMP_METHOD( get_property(const std::string& property,Value& value) )
        COM_IMP_METHOD( set_property(const std::string& property, const Value& value, bool from_script = true) )
        COM_IMP_METHOD( set_property_lua(const std::string& property,const sol::lua_value& value) )
        COM_IMP_METHOD_RET( Value, call_method(const std::string& method, const Value& param) )
        COM_IMP_METHOD_RET( sol::lua_value, call_method_lua(const std::string& method,const sol::lua_value& param) )

        COM_IMP_METHOD( master_loop(bool before_after) )
        COM_IMP_METHOD( process_command(const std::string& command,const Value& param) )

        COM_IMP_METHOD( connect_constraint_output_format(IPin* output_pin, const std::vector<Format> &format) )
        COM_IMP_METHOD( connect_before_match(IFilter *sender_filter) )

        COM_IMP_METHOD( connect(IPin* output_pin,IPin* input_pin) )
        //basic disconnect output pin method #disconnect_output(int32_t output_pin) #disconnect_output() use this implement
        COM_IMP_METHOD( disconnect_output(int32_t output_pin,IPin* input_pin) )
        //disconnect a input pin's  sender, -1 for disconnect all
        COM_IMP_METHOD( disconnect_input(int32_t input_pin) )

        //method must implement by final filters
//        virtual int32_t connect_match_input_format(IPin *sender_pin,IPin *input_pin) = 0;
//        virtual int32_t connect_chose_output_format(IPin* output_pin, int32_t index) = 0;
//        virtual int32_t receive(IPin* input_pin,FramePointer frame) = 0;
//        virtual int32_t requare(int32_t duration,const std::vector<PinIndex>& output_pins) = 0;
    protected:
        IGraph*         graph_ = nullptr;
        sol::table      filter_state_;
        std::string     id_;
        std::string     module_;
        //activable
        bool            activable_ = true;
        //pin level,from render=0 source=n
        int32_t         level_     = 0;
    private:
        PinVector input_pins_;
        PinVector output_pins_;
    };

    typedef std::map<std::string,Value> SolPropertiesMap;

}
#endif // SDP_FILTER_BASE_H
