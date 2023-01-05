#include <exception>
#include <iostream>
#include <string>
#include <GL/glew.h> 
#include <GL/wglew.h> //WGLEW = Windows GL Extension Wrangler (change for different platform) 

#include "globals.h"
#include "lua_engine.h"

// Catch C++ exceptions and convert them to Lua error messages.
// Customize as needed for your own exception classes.
static int wrap_exceptions(lua_State* L, lua_CFunction f)
{
    try {
        return f(L);  // Call wrapped function and return result.
    }
    catch (const char* s) {  // Catch and convert exceptions.
        lua_pushstring(L, s);
    }
    catch (std::exception& e) {
        lua_pushstring(L, e.what());
    }
    catch (...) {
        lua_pushliteral(L, "exception caught (...)");
    }
    return lua_error(L);  // Rethrow as a Lua error.
}

lua_engine::lua_engine()
{
    // All Lua contexts are held in this structure. We work with it almost all the time.
    this->L = luaL_newstate();

    luaL_openlibs(this->L); // Load ALL Lua libraries 

    // Define wrapper function and enable it.
    lua_pushlightuserdata(this->L, (void*)wrap_exceptions);
    lua_pop(this->L, 1);

    std::cout << "[CPP," << glfwGetTime() << "] Lua engine initialized. Release: " << LUA_RELEASE << ", " << LUA_COPYRIGHT << ", " << LUA_AUTHORS << std::endl;
}

int lua_engine::loadfile(const std::string& filename)
{
    int status;

    // Load the file containing the script we are going to run 
    status = luaL_loadfile(this->L, filename.c_str());
    if (status != LUA_OK) {
        // If something went wrong, error message is at the top of  the stack 
        std::cerr << "Couldn't load Lua script file: " << lua_tostring(this->L, -1) << std::endl;
        return EXIT_FAILURE;
    }

    // Register script as a function, so that it can be called repeatedly.
    lua_setglobal(this->L, "LuaMainFunction");

    return EXIT_SUCCESS;
}

int lua_engine::run(void)
{
    int result;

    // Ask Lua to (re)run script (registered as a function)
    lua_getglobal(L, "LuaMainFunction"); 

    result = lua_pcall(this->L, 0, LUA_MULTRET, 0);
    if (result) {
        std::cerr << "Failed to run script: " << lua_tostring(this->L, -1) << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int lua_engine::register_function(const char * lua_fun_name, lua_CFunction c_fun_ptr)
{
    lua_register(this->L, lua_fun_name, c_fun_ptr);
    return 0;
}

std::vector<int> lua_engine::getIntVector(const std::string& name) {
    std::vector<int> v;
    lua_gettostack(name.c_str());
    if (lua_isnil(L, -1)) { // array is not found
        return std::vector<int>();
    }
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        v.push_back((int)lua_tonumber(L, -1));
        lua_pop(L, 1);
    }
    clean();
    return v;
}

std::vector<std::string> lua_engine::getTableKeys(const std::string& name) {
    std::string code =
        "function getKeys(name) "
        "s = \"\""
        "for k, v in pairs(_G[name]) do "
        "    s = s..k..\",\" "
        "    end "
        "return s "
        "end"; // function for getting table keys
    luaL_loadstring(L,
        code.c_str()); // execute code
    lua_pcall(L, 0, 0, 0);
    lua_getglobal(L, "getKeys"); // get function
    lua_pushstring(L, name.c_str());
    lua_pcall(L, 1, 1, 0); // execute function
    std::string test = lua_tostring(L, -1);
    std::vector<std::string> strings;
    std::string temp = "";

    for (unsigned int i = 0; i < test.size(); i++) {
        if (test.at(i) != ',') {
            temp += test.at(i);
        }
        else {
            strings.push_back(temp);
            temp = "";
        }
    }
    clean();
    return strings;
}

lua_engine::~lua_engine()
{
    if (L)
    {
        lua_close(this->L);
        L = nullptr;
    }
}

