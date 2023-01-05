#include "globals.h"
#include "lua_engine.h"
#include "lua_interface.h"


//--------------------------------------------------------------------------------------------------------------------------
// forward declaration of functions to be registered in Lua

static int c_fun_noparams(lua_State* L); // example of:  void function(void)
static int c_fun_param_ret(lua_State* L);// example of:  int function(int,string)

//--------------------------------------------------------------------------------------------------------------------------

int lua_init(const std::string& filename)
{
    globals.lua.loadfile(filename);

    // register Lua <-> C++ interface functions
    globals.lua.register_function("fun_noparams", c_fun_noparams);
    globals.lua.register_function("fun_param_ret", c_fun_param_ret);

    return EXIT_SUCCESS;
}

int lua_pre_run(void)
{
    static bool once = false;

    if (!once) //run this only once 
    {
        once = true;

        // set values for Lua script
        // simple Lua internal state access
        globals.lua.set("e", 666);
    }

	//set tables in Lua
	{
        //nested tables:
        // t     = {}       whole table
        // r[]   = {}    //row table
        // r[][] = {[0]=0, [1]=1, [2]=0, ...}
        // t[]   = {0=r}
        // etc...

		lua_newtable(globals.lua.L); //whole table
		for (int j = 0; j < 5; j++)
		{
			lua_newtable(globals.lua.L); //row table
			for (int i = 0; i < 5; i++)
			{
				globals.lua.push(i);  // push key (stack index -1)
				globals.lua.push((i == j ? glfwGetTime() : 0));  // push value (stack index -2)
				lua_settable(globals.lua.L, -3); //insert into row table
			}
			globals.lua.push(j);           //push key 
			lua_insert(globals.lua.L, -2); // swap table and key (must be: whole table, key, row table)
			lua_settable(globals.lua.L, -3); //insert into whole table
		}
		lua_setglobal(globals.lua.L, "map");
	}

    return EXIT_SUCCESS;
}

int lua_post_run(void)     // get values from executed Lua script
{
    static bool once = false;

    if (!once) //run this only once to prevent noise in terminal
    {
        once = true;
        std::cout << "[CPP," << glfwGetTime() << "] Read Simple Variables" << std::endl;
        std::cout << "[CPP," << glfwGetTime() << "] Got '" << globals.lua.get<int>("a") << "' for variable 'a'." << std::endl;
        std::cout << "[CPP," << glfwGetTime() << "] Got '" << globals.lua.get<int>("b") << "' for variable 'b'." << std::endl;
        std::cout << "[CPP," << glfwGetTime() << "] Got '" << globals.lua.get<int>("c") << "' for variable 'c'." << std::endl;
        std::cout << "[CPP," << glfwGetTime() << "] Got '" << globals.lua.get<float>("d") << "' for variable 'd'." << std::endl;
        std::cout << "[CPP," << glfwGetTime() << "] Got '" << globals.lua.get<float>("e") << "' for variable 'e'." << std::endl;
        std::cout << "[CPP," << glfwGetTime() << "] Got '" << globals.lua.get<std::string>("t") << "' for variable 't'." << std::endl;

        std::cout << "[CPP," << glfwGetTime() << "] Read Tables" << std::endl;
        std::cout << "[CPP," << glfwGetTime() << "] Got '" << globals.lua.get<int>("complex_avatar.bullets") << "' for variable 'complex_avatar.bullets'." << std::endl;
        std::cout << "[CPP," << glfwGetTime() << "] Got '" << globals.lua.get<std::string>("complex_avatar.model") << "' for variable 'complex_avatar.model'." << std::endl;
        std::cout << "[CPP," << glfwGetTime() << "] Got '" << globals.lua.get<float>("complex_avatar.position.x") << "' for variable 'complex_avatar.position.x'." << std::endl;

        // get array from Lua 
        {
            std::cout << "[CPP," << glfwGetTime() << "] Read Array 'simple_array'. Content:" ;
            
            std::vector<int> v = globals.lua.getIntVector("simple_array");
			for (auto i : v) {
                std::cout << i << ",";
            }
            std::cout << std::endl;
        }

        // get table keys from Lua 
        {
            std::cout << "[CPP," << glfwGetTime() << "] Read Table keys of 'avatar'. Content:";

            std::vector<std::string> v = globals.lua.getTableKeys("avatar");
            for (auto & i : v) {
                std::cout << i << " = " << globals.lua.get<int>("avatar."+i);
            }
            std::cout << std::endl;
        }

        //call function in Lua script
        {
            std::cout << "[CPP," << glfwGetTime() << "] Calling Lua:hello(string)'" << std::endl;

            lua_getglobal(globals.lua.L, "hello");
            if (lua_isfunction(globals.lua.L, -1))
            {
                lua_pushstring(globals.lua.L, "'ICP C++'");
                lua_pcall(globals.lua.L, 1, 0, 0);
            }
        }
    }

    return EXIT_SUCCESS;
}

//--------------------------------------------------------------------------------------------------------------------------

static int c_fun_noparams(lua_State* L)
{
    if (lua_gettop(L) != 0)
    {
        lua_pushstring(L, "incorrect argument count in lua function");
        lua_error(L);
    }

    // do something... 
    std::cout << "[CPP," << glfwGetTime() << "] Lua script called function :" << __func__ << std::endl;

    return 0; // return nothing: return stack size == 0
}

static int c_fun_param_ret(lua_State* L)
{
    const char* s;  // pointer to access string passed from Lua script
    lua_Number n;
    double retval;

    if (lua_gettop(L) != 2)
    {
        lua_pushstring(L, "incorrect argument count in lua function");
        lua_error(L);
    }

    // 1st argument must be number
    luaL_checktype(L, 1, LUA_TNUMBER);
    
    // 2nd argument must be string
    luaL_checktype(L, 2, LUA_TSTRING);

    n = lua_tonumber(L, 1);
    s = lua_tostring(L, 2);

    if (!s)
    {
        lua_pushstring(L, "no string generated?");
        lua_error(L);
    }

    //do something
    retval = glfwGetTime();  //call some internal function to generate value
    std::cout << "[CPP," << retval << "] Lua script called function :" << __func__ << std::endl;
    std::cout << "[CPP," << retval << "] Got arguments: " << n << ", " << s << std::endl;

    lua_pushnumber(L, retval);

    return 1; //returning single value
}

