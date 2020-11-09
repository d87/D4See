#pragma once
#include <string>

extern "C" {
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}
#pragma comment(lib, "lua.lib")

extern lua_State* L;

lua_State* InitLua();
int LoadScript(lua_State* L, std::string filename);