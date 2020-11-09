#include "D4See.h"
#include "LuaVM.h"
#include "WindowManager.h"

lua_State* L;

int LoadScript(lua_State* L, std::string filename) {
	auto root = GetExecutableDir();
	std::filesystem::path file(filename);
	std::filesystem::path path = root / file;

	if (luaL_loadfile(L, path.string().c_str()) || lua_pcall(L, 0, 0, 0)) {
		LOG_ERROR("cannot run configuration file: %s\r\n", lua_tostring(L, -1));
		return 0;
	}
	return 1;
}

static int l_BindAction(lua_State* L) {
	std::string keyStr = luaL_checkstring(L, 1);
	std::string action = luaL_checkstring(L, 2);
	gWinMgr.input.BindKey(keyStr, action);
	return 1;
}

static const luaL_Reg loadedlibs[] = {
  {LUA_GNAME, luaopen_base},
  //{LUA_LOADLIBNAME, luaopen_package},
  {LUA_COLIBNAME, luaopen_coroutine},
  {LUA_TABLIBNAME, luaopen_table},
  //{LUA_IOLIBNAME, luaopen_io},
  //{LUA_OSLIBNAME, luaopen_os},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_MATHLIBNAME, luaopen_math},
  {LUA_UTF8LIBNAME, luaopen_utf8},
  {LUA_DBLIBNAME, luaopen_debug},
  {NULL, NULL}
};

lua_State* InitLua() {
	lua_State* L = luaL_newstate();

	//luaL_openlibs(L); // Standard function to load all libs
	// Loading standard Lua libraries
	const luaL_Reg* lib;
	for (lib = loadedlibs; lib->func; lib++) {
		luaL_requiref(L, lib->name, lib->func, 1);
		lua_pop(L, 1);  /* remove lib */
	}

	lua_register(L, "BindAction", l_BindAction);
	//lua_register(L, "UnregisterEvent", l_UnregisterEvent);
	//lua_register(L, "AddScript", l_AddScript);
	//lua_register(L, "Shell", l_shell);
	//lua_register(L, "sh", l_shell);
	//lua_register(L, "Start", l_Start);
	//lua_register(L, "Shutdown", l_Shutdown);
	//lua_register(L, "GetWindowTitle", l_GetWindowTitle);
	//lua_register(L, "RemoveMenu", l_RemoveMenu);
	//lua_register(L, "MouseInput", l_MouseInput);
	//lua_register(L, "KeyboardInput", l_KeyboardInput);
	//lua_register(L, "GetCursorPos", l_GetCursorPos);
	//lua_register(L, "IsPressed", l_IsPressed);
	//lua_register(L, "RegisterHotKey", l_RegisterHotKey);
	//lua_register(L, "GetWindowProcess", l_GetWindowProcess);
	//lua_register(L, "CreateTimer", l_CreateTimer);
	//lua_register(L, "KillTimer", l_KillTimer);
	//lua_register(L, "SetKeyDelay", l_SetKeyDelay);
	//lua_register(L, "TurnOffMonitor", l_TurnOffMonitor);
	//lua_register(L, "Reload", l_Reload);
	//lua_register(L, "SetAlwaysOnTop", l_SetAlwaysOnTop);
	//lua_register(L, "IsAlwaysOnTop", l_IsAlwaysOnTop);
	//lua_register(L, "EnableMouseHooks", l_EnableMouseHooks);
	//lua_register(L, "DisableMouseHooks", l_DisableMouseHooks);
	//lua_register(L, "OSDTextLong", l_OSDTextLong);
	//lua_register(L, "GetJoyPosInfo", l_GetJoyPosInfo);
	//lua_register(L, "IsJoyButtonPressed", l_IsJoyButtonPressed);
	//lua_register(L, "SetGamepadVibration", l_SetGamepadVibration);
	//lua_register(L, "IsXInputEnabled", l_IsXInputEnabled);
	//lua_register(L, "GetMasterVolume", l_GetMasterVolume);
	//lua_register(L, "SetMasterVolume", l_SetMasterVolume);
	//lua_register(L, "GetMasterVolumeMute", l_GetMasterVolumeMute);
	//lua_register(L, "SetMasterVolumeMute", l_SetMasterVolumeMute);
	//lua_register(L, "ToggleWindowTitle", l_ToggleWindowTitle);
	//lua_register(L, "GetClipboardText", l_GetClipboardText);
	//lua_register(L, "ListWindows", l_ListWindows);
	//lua_register(L, "GetWindowPos", l_GetWindowPos);
	//lua_register(L, "SetWindowPos", l_SetWindowPos);
	//lua_register(L, "ShowWindow", l_ShowWindow);
	//lua_register(L, "GetMouseSpeed", l_GetMouseSpeed);
	//lua_register(L, "SetMouseSpeed", l_SetMouseSpeed);

	//lua_register(L, "GetSelectedGamepad", l_GetSelectedGamepad);
	//lua_register(L, "SelectGamepad", l_SelectGamepad);

	//lua_register(L, "PlaySound", l_PlaySound);
	//lua_register(L, "SwitchToDesktop", l_SwitchToDesktop);
	//lua_register(L, "MoveWindowToDesktop", l_MoveWindowToDesktop);
	//lua_register(L, "TogglePinCurrentWindow", l_TogglePinCurrentWindow);
	//lua_register(L, "IsWindowMaximized", l_IsWindowMaximized);
	//lua_register(L, "GetDesktopCount", l_GetDesktopCount);
	//lua_register(L, "GetCurrentDesktopNumber", l_GetCurrentDesktopNumber);
	//lua_register(L, "MakeBorderless", l_MakeBorderless);
	//lua_register(L, "explore", l_explore);
	//lua_register(L, "GetMonitorBrightness", l_GetMonitorBrightness);
	//lua_register(L, "SetMonitorBrightness", l_SetMonitorBrightness);
	//lua_register(L, "GetActiveExplorerWindowPath", l_GetActiveExplorerWindowPath);





	//lua_register(L, "CreateFrame", l_CreateFrame);

	//lua_newtable(L);
	//lua_setglobal(L, "console");
	//lua_getglobal(L, "console");
	//lua_pushboolean(L, 1);
	//lua_setfield(L, -2, "enabled");
	////lua_pushboolean(L, 0);
	////lua_setfield(L, -2, "init");
	//lua_pushnumber(L, 300);
	//lua_setfield(L, -2, "width");
	//lua_pushnumber(L, 400);
	//lua_setfield(L, -2, "height");
	//lua_pushcfunction(L, l_SetConsoleBackgroundColor);
	//lua_setfield(L, -2, "SetBackgroundColor");
	//lua_pushcfunction(L, l_SetConsoleTextColor);
	//lua_setfield(L, -2, "SetColor");
	//lua_pushcfunction(L, l_ShowConsole);
	//lua_setfield(L, -2, "Show");
	//lua_pushcfunction(L, l_HideConsole);
	//lua_setfield(L, -2, "Hide");
	//lua_pushcfunction(L, l_ClearConsole);
	//lua_setfield(L, -2, "Clear");
	return L;
}