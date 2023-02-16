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

namespace sol {

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

    sol::state& state(){return  *lua_state_;}
    operator sol::state&(){return  *lua_state_;}
    operator std::shared_ptr<sol::state> (){return lua_state_;}
private:
    std::shared_ptr<sol::state> lua_state_;
    std::string error_handler_function_;
};

}

#endif // LUA_EXPRESSION_H
