#include "LuaVM.h"
#include "D4See.h"
#include "Configuration.h"



int D4See::Configuration::LoadFile(lua_State* L, const std::string& filename) {
	auto root = GetExecutableDir();
	std::filesystem::path file(filename);
	std::filesystem::path path = root / file;

	if (luaL_loadfile(L, path.string().c_str()) || lua_pcall(L, 0, 0, 0)) {
		LOG_ERROR("cannot run configuration file: {0}\r\n", lua_tostring(L, -1));
		return 0;
	}


	lua_getglobal(L, "CONFIG");
	if (!lua_istable(L, -1)) {
		LOG_ERROR("CONFIG table is missing\r\n");
		return 0;
	}

	lua_getfield(L, -1, "Borderless");
	this->isBorderless = lua_toboolean(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, -1, "SortMethod");
	std::string sortMethodStr(luaL_checkstring(L, -1));
	this->sortMethod = (sortMethodStr == "ByDateModified") ? PlaylistSortMethod::ByDateModified : PlaylistSortMethod::ByName;
	lua_pop(L, 1);

	lua_getfield(L, -1, "StretchToScreenWidth");
	this->stretchToScreenWidth = lua_toboolean(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, -1, "StretchToScreenHeight");
	this->stretchToScreenHeight = lua_toboolean(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, -1, "ShrinkToScreenWidth");
	this->shrinkToScreenWidth = lua_toboolean(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, -1, "ShrinkToScreenHeight");
	this->shrinkToScreenHeight = lua_toboolean(L, -1);
	lua_pop(L, 1);

	return 1;
};