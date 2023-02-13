#ifndef LUA_EXPRESSION_H
#define LUA_EXPRESSION_H
#include <any>
#include <stdint.h>
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <sstream>
#include <sol/sol.hpp>
#define LUA_PROTECT_FUNC(obj,func) (sol::protected_function(obj[func]))
#define NULL_PTR            ((void*)nullptr)
#define NULL_STRING         ((char*)"")
#define NULL_NUMBER_VEC     std::vector<double>()
#define NULL_LUA_FUNCTION   sol::function()
#define NULL_LUA_TABLE      sol::table()

#define ANY2S64(V)      std::any_cast<int64_t>(V)
#define ANY2U64(V)      std::any_cast<uint64_t>(V)
#define ANY2F64(V)      std::any_cast<double>(V)
#define ANY2BOOL(V)     std::any_cast<bool>(V)
#define ANY2STR(V)      std::any_cast<const std::string&>(V)
#define ANY2F64ARR(V)   std::any_cast<const std::vector<double>&>(V)
#define ANY2STRARR(V)   std::any_cast<const std::vector<std::string>&>(V)
#define ANY2PTR(V)      std::any_cast<void*>(V)
#define ANY2LUAFUN(V)   std::any_cast<sol::function>(V)
#define ANY2LUATABLE(V) std::any_cast<sol::table>(V)

namespace sol {

enum NativeValueType : int32_t{
    kNumber,
    kString,
    kBool,
    kNumberArray,
    kStringArray,
    kPointer,
    kFunction,
    kTable,
};

struct NativeValue{    
    NativeValue(){}
    NativeValue(double v,const std::string& describe = "",bool read_only = false){
        type_ = kNumber;
        describe_ = describe;
        read_only_ = read_only;
        value_ = v;
    }
    NativeValue(int v,const std::string& describe = "",bool read_only = false)
        :NativeValue(double(v),describe,read_only){
        //NOTE: ALL NUMBER USE DOUBLE TYPE
    }
    NativeValue(int64_t v,const std::string& describe = "",bool read_only = false)
        :NativeValue(double(v),describe,read_only){
        //NOTE: ALL NUMBER USE DOUBLE TYPE
    }
    NativeValue(bool v,const std::string& describe = "",bool read_only = false){
        type_ = kBool;
        describe_ = describe;
        read_only_ = read_only;
        value_ = v;
    }
    NativeValue(void* v,const std::string& describe = "",bool read_only = false){
        type_ = kPointer;
        describe_ = describe;
        read_only_ = read_only;
        value_ = v;
    }
    NativeValue(const std::vector<double>& v,const std::string& describe = "",bool read_only = false){
        type_ = kNumberArray;
        describe_ = describe;
        read_only_ = read_only;
        value_ = v;
    }
    NativeValue(const std::string& v,const std::string& describe = "",bool read_only = false){
        type_ = kString;
        describe_ = describe;
        read_only_ = read_only;
        value_ = v;
    }
    NativeValue(const char* v,const std::string& describe = "",bool read_only = false){
        type_ = kString;
        describe_ = describe;
        read_only_ = read_only;
        value_ = v;
    }
    NativeValue(const std::vector<std::string>& v,const std::string& describe = "",bool read_only = false){
        type_ = kStringArray;
        describe_ = describe;
        read_only_ = read_only;
        value_ = v;
    }
    NativeValue(sol::function v,const std::string& describe = "",bool read_only = false){
        type_ = kFunction;
        describe_ = describe;
        read_only_ = read_only;
        value_ = v;
    }
    NativeValue(sol::table v,const std::string& describe = "",bool read_only = false){
        type_ = kTable;
        describe_ = describe;
        read_only_ = read_only;
        value_ = v;
    }
    NativeValue(const NativeValue& v){
        type_ = v.type_;
        value_ = v.value_;
        describe_ = v.describe_;
        read_only_ = v.read_only_;
    }

    void set_value(const sol::NativeValue &v)
    {
        if(type_ != v.type_)
            return;
        value_ = v.value_;
        describe_ = v.describe_;
    }

    void from_double(double v){
        type_ = kNumber;
        value_changed_ = (ANY2F64(value_) != v);
        value_ = v;
    }
    void from_bool(bool v){
        type_ = kBool;
        value_changed_ = (ANY2BOOL(value_) != v);
        value_ = v;
    }

    void from_vector(const std::vector<double>& v){
        type_ = kNumberArray;
        value_changed_ = (ANY2F64ARR(value_) != v);
        value_ = v;
    }
    void from_vector(const std::vector<std::string>& v){
        type_ = kStringArray;
        value_changed_ = (ANY2STRARR(value_) != v);
        value_ = v;
    }
    void from_string(const std::string& v){
        type_ = kString;
        value_changed_ = (ANY2STR(value_) != v);
        value_ = v;
    }
    void from_pointer(void* v){
        type_ = kPointer;
        value_changed_ = (ANY2PTR(value_) != v);
        value_ = v;
    }
    void from_function(sol::function v){
        type_ = kFunction;
        auto fun = ANY2LUAFUN(value_);
        if(v.valid() && fun.valid())
            value_changed_ = sol::operator==(fun,v);
        else if(v.valid() != fun.valid())
            value_changed_ = true;
        else
            value_changed_ = false;
        if(value_changed_)
            value_ = v;
    }

    void from_function(sol::table v){
        type_ = kTable;
        auto tb = ANY2LUATABLE(value_);
        if(v.valid() && tb.valid())
            value_changed_ = sol::operator==(tb,v);
        else if(v.valid() != tb.valid())
            value_changed_ = true;
        else
            value_changed_ = false;
        if(value_changed_)
            value_ = v;
    }

    void operator=(double v){
        from_double(v);
    }
    void operator=(int64_t v){
        from_double(v);
    }
    void operator=(int v){
        from_double(v);
    }
    void operator=(bool v){
        from_bool(v);
    }
    void operator=(const std::vector<double> v){
        from_vector(v);
    }
    void operator=(const std::vector<std::string> v){
        from_vector(v);
    }
    void operator=(const std::string& v){
        from_string(v);
    }
    void operator=(void* v){
        from_pointer(v);
    }
    void operator=(sol::function v){
        from_function(v);
    }
    void operator=(sol::table v){
        from_function(v);
    }
    void operator=(const sol::lua_value &value)
    {
        set_lua_value_with_check(value);
    }

    operator double() const{
        return ANY2F64(value_);
    }
    operator int() const{
        return ANY2F64(value_);
    }
    operator int64_t() const{
        return ANY2F64(value_);
    }
    operator bool() const{
        return ANY2BOOL(value_);
    }
    operator const std::string&() const{
        const std::string& s = ANY2STR(value_);
        return s;
    }
    operator const std::vector<double>&() const{
        return ANY2F64ARR(value_);
    }
    operator const std::vector<std::string>&() const{
        return ANY2STRARR(value_);
    }
    operator void* () const{
        return ANY2PTR(value_);
    }
    operator sol::function() const{
        return ANY2LUAFUN(value_);
    }
    operator sol::table() const{
        return ANY2LUATABLE(value_);
    }

    double as_double() const{
        return ANY2F64(value_);
    }
    int as_int() const{
        return ANY2F64(value_);
    }
    int64_t as_int64() const{
        return ANY2F64(value_);
    }
    bool as_bool() const{
        return ANY2BOOL(value_);
    }
    const std::string& as_string() const{
        const std::string& s = ANY2STR(value_);
        return s;
    }
    const std::vector<double>& as_double_vector() const{
        return ANY2F64ARR(value_);
    }
    const std::vector<std::string>& as_string_vector() const{
        return ANY2STRARR(value_);
    }
    void* as_pointer() const{
        return ANY2PTR(value_);
    }
    sol::function as_function() const{
        return ANY2LUAFUN(value_);
    }
    sol::table as_table() const{
        return ANY2LUATABLE(value_);
    }

    int32_t set_lua_value_with_check(const sol::lua_value &value)
    {
        do{
            auto value_type = value.value().get_type();
            if(value_type == sol::type::number){
                if(type_ != kNumber)
                    break;
                *this = value.as<double>();
            }
            else if(value_type == sol::type::boolean){
                if(type_ != kBool)
                    break;
                *this = value.as<bool>();
            }
            else if(value_type == sol::type::string){
                if(type_ != kString)
                    break;
                *this = value.as<std::string>();
            }
            else if(value_type == sol::type::userdata || value_type == sol::type::lightuserdata){
                if(type_ != kPointer)
                    break;
                *this = value.as<void*>();
            }
            else if(value_type == sol::type::table){
                if(type_ != kNumberArray && type_ != kTable)
                    break;
                auto tb = value.as<sol::table>();
                std::vector<double> numbers = tb.as<std::vector<double>>();
                if(numbers.size())
                    *this = tb.as<std::vector<double>>();
                else if(!tb.empty())
                    *this = tb;
                else
                    break;
            }
            else if(value_type == sol::type::function){
                if(type_ != kFunction)
                    break;
                *this = value.as<sol::function>();
            }
            else
                return -1;            
            return 0;
        }while (false);

        return -1;
    }

    int32_t create_from_lua_value(const sol::lua_value &value)
    {
        do{
            auto value_type = value.value().get_type();
            if(value_type == sol::type::number){
                *this = value.as<double>();
                return 0;
            }
            else if(value_type == sol::type::boolean){
                *this = value.as<bool>();
                return 0;
            }
            else if(value_type == sol::type::string){
                *this = value.as<std::string>();
                return 0;
            }
            else if(value_type == sol::type::userdata || value_type == sol::type::lightuserdata){
                *this = value.as<void*>();
                return 0;
            }
            else if(value_type == sol::type::table){
                auto tb = value.as<sol::table>();
                if(tb.size()){
                    if(tb[0].get_type() == sol::type::number){
                        *this = tb.as<std::vector<double>>();
                    }
                    else if(tb[0].get_type() == sol::type::string){
                        *this = tb.as<std::vector<std::string>>();
                    }
                    else{
                        *this = tb;
                    }
                }
                return 0;
            }
            else if(value_type == sol::type::function){
                *this = value.as<sol::function>();
            }
        }while (false);

        return -1;
    }
    lua_value create_lua_value(const state& lua_state) const{
        switch (type_) {
        case kNumber:  return sol::lua_value(lua_state,ANY2F64(value_));
        case kBool:    return sol::lua_value(lua_state,ANY2BOOL(value_));
        case kString:  return sol::lua_value(lua_state,ANY2STR(value_));
        case kNumberArray:  return sol::lua_value(lua_state,ANY2F64ARR(value_));
        case kStringArray:  return sol::lua_value(lua_state,ANY2STRARR(value_));
        case kPointer: return sol::lua_value(lua_state,ANY2PTR(value_));
        case kFunction:return sol::lua_value(lua_state,ANY2LUAFUN(value_));
        case kTable:   return sol::lua_value(lua_state,ANY2LUATABLE(value_));
        }
        return sol::lua_value(lua_state,0);
    }
    std::string printable()
    {
        std::ostringstream ss;
        if(type_ == kNumber)
            ss<<"Number:"<<kNumber;
        else if(type_ == kBool)
            ss<<"Bool:"<<((ANY2BOOL(value_))?"true":"false");
        else if(type_ == kNumberArray){
            bool first = true;
            ss<<"Number Arrays:[";
            for(double v : ANY2F64ARR(value_))
                ss<<(first?"":",")<<v,first=false;
            ss<<"]";
        }
        else if(type_ == kStringArray){
            bool first = true;
            ss<<"Number Arrays:[";
            for(const std::string& v : ANY2STRARR(value_))
                ss<<(first?"":",")<<"\""<<v<<"\"",first=false;
            ss<<"]";
        }
        else if(type_ == kString)
            ss<<"String:"<<ANY2STR(value_);
        else if(type_ == kPointer)
            ss<<"Pointer:"<<ANY2PTR(value_);
        else if(type_ == kFunction)
            ss<<"Function:"<<(ANY2LUAFUN(value_).valid()?"valid":"nil");
        else if(type_ == kTable)
            ss<<"Table:"<<(ANY2LUATABLE(value_).valid()?"valid":"nil");
        return ss.str();

    }


    NativeValueType         type_    = kNumber;
    std::any                value_;
    std::string             describe_;
    bool                    read_only_      = false;
    bool                    value_changed_  = false;
};

class LuaOperator
{
public:

    LuaOperator()
    {

    }
    ~LuaOperator()
    {

    }

    int32_t create()
    {
        lua_state_ = std::shared_ptr<sol::state>(new sol::state());
        lua_state_->open_libraries();
        return 0;
    }
    int32_t attch(std::shared_ptr<sol::state> other)
    {
        lua_state_ = other;
        return 0;
    }

    int32_t apply_script(const std::string &script)
    {
        try{
            lua_state_->safe_script_file(script);
        }
        catch(sol::error& error){
            fprintf(stderr,"LUA RUN SCRIPT ERROR: %s\n\t",error.what());
            return -1;
        }
        return 0;
    }

    int32_t apply_symbols(std::map<std::string, NativeValue *> &symbols)
    {
        for(auto& item : symbols){
            apply_symbol(item.first,item.second);
        }
        symbols_ = &symbols;
        return 0;
    }

    int32_t apply_symbol(const std::string name, NativeValue *symbol)
    {
        if(symbol->type_ == kNumber){
            double v = *symbol;
            (*lua_state_)[name] = v;
        }
        else if(symbol->type_ == kNumberArray){
            std::vector<double> v = *symbol;
            (*lua_state_)[name] = sol::as_table(v);
        }
        else if(symbol->type_ == kString){
            std::string v = *symbol;
            (*lua_state_)[name] = v;
        }
        return 0;
    }

    void set_error_handler_function(const std::string &function){
        error_handler_function_ = function;
    }

    template <typename... Args>
    protected_function_result operator()(const std::string &function,Args&&... args) const {
            sol::protected_function func = get_function(function);
            return func.call<>(std::forward<Args>(args)...);
    }

    template <typename... Args>
    protected_function_result operator()(sol::table obj,const std::string &function,Args&&... args) const {
            sol::protected_function func = get_member_function(obj,function);
            return func.call<>(std::forward<Args>(args)...);
    }

    sol::protected_function get_function(const std::string &function) const {
        sol::protected_function func;
        if(error_handler_function_.empty())
            func = (*lua_state_)[function];
        else
            func = sol::protected_function((*lua_state_)[function],(*lua_state_)[error_handler_function_]);
        return func;
    }
    sol::protected_function get_member_function(sol::table obj,const std::string &function) const{
        sol::protected_function func;
        if(error_handler_function_.empty())
            func = obj[function];
        else
            func = sol::protected_function(obj[function],(*lua_state_)[error_handler_function_]);
        return func;
    }

    sol::lua_value create_value(const NativeValue &value)
    {
        return value.create_lua_value(*lua_state_);
    }

    sol::state& state(){return  *lua_state_;}
    operator sol::state&(){return  *lua_state_;}
    operator std::shared_ptr<sol::state> (){return lua_state_;}
private:
    std::map<std::string,NativeValue*>* symbols_ = nullptr;
    std::shared_ptr<sol::state> lua_state_;
    std::string error_handler_function_;
};

}

#endif // LUA_EXPRESSION_H
