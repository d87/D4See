#pragma once
#include <string>
#include "LuaVM.h"
#include "util.h"
#include "playlist.h"

namespace D4See {

	class Configuration {
	public:
		bool stretchToScreenWidth = true;
		bool stretchToScreenHeight = false;
		bool shrinkToScreenWidth = false;
		bool shrinkToScreenHeight = false;
		bool isFullscreen = false;
		bool isBorderless = false;
		PlaylistSortMethod sortMethod = PlaylistSortMethod::ByDateModified;

		Configuration() {};
		int LoadFile(lua_State* L, const std::string& filename);
	};
}