/*
 * lua_engine.h
 * JJ
 */

#ifndef LUA_ENGINE_H_
#define LUA_ENGINE_H_

#include <string>
#include <lua542/lua.hpp>

class lua_engine
{
	int level;
public:
	lua_State* L = nullptr;

	lua_engine();
	~lua_engine();

	int loadfile(const std::string& filename);
	int run(void);
	int register_function(const char* lua_fun_name, lua_CFunction c_fun_ptr);

	std::vector<int> getIntVector(const std::string& name);
	std::vector<std::string> getTableKeys(const std::string& name);

	inline void clean() {
		int n = lua_gettop(L);
		lua_pop(L, n);
	}

	void printError(const std::string& variableName, const std::string& reason) {
		std::cout << "[CPP] Error: can't get [" << variableName << "] variable. " << reason << std::endl;
	}

	// templated get
	template<typename T>
	T get(const std::string& variableName) {
		if (!L) {
			printError(variableName, "Script is not loaded");
			return lua_getdefault<T>();
		}

		if (variableName.empty()) {
			printError(variableName, "Empty variable name");
			return lua_getdefault<T>();
		}

		T result;
		
		if (lua_gettostack(variableName)) { // variable succesfully on top of stack
			result = lua_get<T>(variableName);
		}
		else {
			result = lua_getdefault<T>();
		}

		clean();
		return result;
	}

	bool lua_gettostack(const std::string& variableName) {
		level = 0;
		std::string var = "";
		for (unsigned int i = 0; i < variableName.size(); i++) {
			if (variableName.at(i) == '.') {
				if (level == 0) {
					lua_getglobal(L, var.c_str());
				}
				else {
					lua_getfield(L, -1, var.c_str());
				}

				if (lua_isnil(L, -1)) {
					printError(variableName, var + " is not defined");
					return false;
				}
				else {
					var = "";
					level++;
				}
			}
			else {
				var += variableName.at(i);
			}
		}
		if (level == 0) {
			lua_getglobal(L, var.c_str());
		}
		else {
			lua_getfield(L, -1, var.c_str());
		}
		if (lua_isnil(L, -1)) {
			printError(variableName, var + " is not defined");
			return false;
		}

		return true;
	}

	// Generic get
	template<typename T>
	T lua_get(const std::string& variableName) { return 0; }

	// Generic default get
	template<typename T>
	T lua_getdefault() { return 0; }
//------------------------------------------------------------------------------------
	template <typename T>
	bool push(const T a)
	{
		if (!this->L) {
			printError("PUSH", "Script is not loaded");
			return false;;
		}

		if constexpr (std::is_same_v<T, int>)
			lua_pushinteger(this->L, a);
		else if  constexpr (std::is_same_v<T, float> or std::is_same_v<T, double>)
			lua_pushnumber(this->L, a);
		else if  constexpr (std::is_same_v<T, bool>)
			lua_pushboolean(this->L, a);
		else if  constexpr (std::is_same_v<T, std::string>)
			lua_pushlstring(this->L, a.c_str(), a.length());
		else if  constexpr (std::is_same_v<T, const char *>)
			lua_pushlstring(this->L, a, strlen(a));
		else
			return false;
		return true;
	}

	// templated set
	template<typename T>
	bool set(const std::string& variableName, const T& value) {
		if (!this->L) {
			printError(variableName, "Script is not loaded");
			return false;;
		}

		if (variableName.empty()) {
			printError(variableName, "Empty variable name.");
			return false;
		}

		if (!push(value))
			return false;

		lua_setglobal(this->L, variableName.c_str());
		return true;
	}

	// Generic set
	template <typename T>
	bool lua_set(const std::string& variableName, T& value) { return; };
};

// Get Specializations

template <>
inline bool lua_engine::lua_get<bool>(const std::string& variableName) {
	return (bool)lua_toboolean(L, -1);
}

template <>
inline int lua_engine::lua_get<int>(const std::string& variableName) {
	if (!lua_isnumber(L, -1)) {
		printError(variableName, "Not a number");
	}
	return (int)lua_tonumber(L, -1);
}

template <>
inline float lua_engine::lua_get<float>(const std::string& variableName) {
	if (!lua_isnumber(L, -1)) {
		printError(variableName, "Not a number");
	}
	return (float)lua_tonumber(L, -1);
}

template <>
inline double lua_engine::lua_get<double>(const std::string& variableName) {
	if (!lua_isnumber(L, -1)) {
		printError(variableName, "Not a number");
	}
	return (double)lua_tonumber(L, -1);
}

template <>
inline std::string lua_engine::lua_get<std::string>(const std::string& variableName) {
	std::string s = "null";
	if (lua_isstring(L, -1)) {
		s = std::string(lua_tostring(L, -1));
	}
	else {
		printError(variableName, "Not a string");
	}
	return s;
}

template<>
inline std::string lua_engine::lua_getdefault<std::string>() {
	return "null";
}

#endif /* LUA_ENGINE_H_ */
