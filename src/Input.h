#pragma once
#include <windows.h>
#include <windowsx.h>
#include <unordered_map>
#include "Action.h"

namespace D4See {
	struct ActiveKeyPress {
		uint8_t VKey;
		uint8_t modMask;
		Action action;
	};

	class InputHandler {
		//std::unordered_map<uint16_t, Action> kbBindTable;
		Action kbBindTable[2048]; // 8 bits for 256 VKeys and 3 bits for modifier states, WIN key isn't used
		callback_function currentMouseMoveCallback;
		std::vector<ActiveKeyPress> pressedActions;

		public:
			InputHandler();
			int ProcessInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
			uint8_t GetModifierMask();
			void FireAction(uint8_t VKey, int isDown);
			int BindKey(std::string& keyStr, const std::string& actionStr);
	};
}

// Using a single vk table for both keys and mouse button presses
enum MBVKeys {
	MBUTTON1 = VK_LBUTTON,
	MBUTTON2 = VK_RBUTTON,
	MBUTTON3 = VK_MBUTTON,
	MBUTTON4 = VK_XBUTTON1,
	MBUTTON5 = VK_XBUTTON2,
	MBUTTON1_DBL = 0x15,
	MBUTTON2_DBL = 0x16,
	MBUTTON3_DBL = 0x17,
	MBUTTON4_DBL = 0x18,
	MBUTTON5_DBL = 0x19,
	MWHEELUP = 0x1A,
	MWHEELDOWN = 0x1C
};

const std::unordered_map<std::string, uint8_t> keyMap = {
	// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
	{ "MBUTTON1", VK_LBUTTON },
	{ "MBUTTON2", VK_RBUTTON },
	//{ "CANCEL", VK_CANCEL },
	{ "MBUTTON3", VK_MBUTTON },
	{ "MBUTTON4", VK_XBUTTON1 },
	{ "MBUTTON5", VK_XBUTTON2 },
	{ "BACKSPACE", VK_BACK },
	{ "TAB", VK_TAB },
	{ "CLEAR", VK_CLEAR },
	{ "RETURN", VK_RETURN },

	{ "SHIFT", VK_SHIFT },
	{ "CTRL", VK_CONTROL },
	{ "ALT", VK_MENU },
	{ "PAUSE", VK_PAUSE },
	{ "CAPSLOCK", VK_CAPITAL },

	// These are special virtual "keys" for mouse button doubleclicks
	{ "MBUTTON1_DBL", 0x15 }, //{ "KANA", VK_KANA },
	{ "MBUTTON2_DBL", 0x16}, //{ "HANGEUL", VK_HANGEUL },
	{ "MBUTTON3_DBL", 0x17}, //{ "HANGUL", VK_HANGUL },
	{ "MBUTTON4_DBL", 0x18 }, //{ "JUNJA", VK_JUNJA },
	{ "MBUTTON5_DBL", 0x19 }, //{ "FINAL", VK_FINAL },
	{ "MWHEELUP", 0x1A }, //{ "HANJA", VK_HANJA },
	{ "ESC", VK_ESCAPE }, //0x1B
	{ "MWHEELDOWN", 0x1C }, //{ "KANJI", VK_KANJI },
	//{ "CONVERT", VK_CONVERT },
	//{ "NONCONVERT", VK_NONCONVERT },
	//{ "ACCEPT", VK_ACCEPT },
	//{ "MODECHANGE", VK_MODECHANGE },

	{ "SPACE", VK_SPACE },
	{ "PAGEUP", VK_PRIOR },
	{ "PAGEDOWN", VK_NEXT },
	{ "END", VK_END },
	{ "HOME", VK_HOME },
	{ "LEFT", VK_LEFT },
	{ "UP", VK_UP },
	{ "RIGHT", VK_RIGHT },
	{ "DOWN", VK_DOWN },
	//{ "SELECT", VK_SELECT },
	//{ "PRINT", VK_PRINT },
	//{ "EXECUTE", VK_EXECUTE },
	{ "PRINTSCREEN", VK_SNAPSHOT },
	{ "INSERT", VK_INSERT },
	{ "DELETE", VK_DELETE },
	{ "DEL", VK_DELETE },
	{ "HELP", VK_HELP },


	{ "0", 0x30 },
	{ "1", 0x31 },
	{ "2", 0x32 },
	{ "3", 0x33 },
	{ "4", 0x34 },
	{ "5", 0x35 },
	{ "6", 0x36 },
	{ "7", 0x37 },
	{ "8", 0x38 },
	{ "9", 0x39 },
	{ "A", 0x41 },
	{ "B", 0x42 },
	{ "C", 0x43 },
	{ "D", 0x44 },
	{ "E", 0x45 },
	{ "F", 0x46 },
	{ "G", 0x47 },
	{ "H", 0x48 },
	{ "I", 0x49 },
	{ "J", 0x4A },
	{ "K", 0x4B },
	{ "L", 0x4C },
	{ "M", 0x4D },
	{ "N", 0x4E },
	{ "O", 0x4F },
	{ "P", 0x50 },
	{ "Q", 0x51 },
	{ "R", 0x52 },
	{ "S", 0x53 },
	{ "T", 0x54 },
	{ "U", 0x55 },
	{ "V", 0x56 },
	{ "W", 0x57 },
	{ "X", 0x58 },
	{ "Y", 0x59 },
	{ "Z", 0x5A },


	{ "LWIN", VK_LWIN },
	{ "RWIN", VK_RWIN },
	//{ "APPS", VK_APPS },

	{ "SLEEP", VK_SLEEP },

	{ "NUMPAD0", VK_NUMPAD0 },
	{ "NUMPAD1", VK_NUMPAD1 },
	{ "NUMPAD2", VK_NUMPAD2 },
	{ "NUMPAD3", VK_NUMPAD3 },
	{ "NUMPAD4", VK_NUMPAD4 },
	{ "NUMPAD5", VK_NUMPAD5 },
	{ "NUMPAD6", VK_NUMPAD6 },
	{ "NUMPAD7", VK_NUMPAD7 },
	{ "NUMPAD8", VK_NUMPAD8 },
	{ "NUMPAD9", VK_NUMPAD9 },
	{ "MULTIPLY", VK_MULTIPLY },
	{ "ADD", VK_ADD },
	{ "SEPARATOR", VK_SEPARATOR },
	{ "SUBTRACT", VK_SUBTRACT },
	{ "DECIMAL", VK_DECIMAL },
	{ "DIVIDE", VK_DIVIDE },
	{ "F1", VK_F1 },
	{ "F2", VK_F2 },
	{ "F3", VK_F3 },
	{ "F4", VK_F4 },
	{ "F5", VK_F5 },
	{ "F6", VK_F6 },
	{ "F7", VK_F7 },
	{ "F8", VK_F8 },
	{ "F9", VK_F9 },
	{ "F10", VK_F10 },
	{ "F11", VK_F11 },
	{ "F12", VK_F12 },
	{ "F13", VK_F13 },
	{ "F14", VK_F14 },
	{ "F15", VK_F15 },
	{ "F16", VK_F16 },
	{ "F17", VK_F17 },
	{ "F18", VK_F18 },
	{ "F19", VK_F19 },
	{ "F20", VK_F20 },
	{ "F21", VK_F21 },
	{ "F22", VK_F22 },
	{ "F23", VK_F23 },
	{ "F24", VK_F24 },

	//{ "NAVIGATION_VIEW", VK_NAVIGATION_VIEW },
	//{ "NAVIGATION_MENU", VK_NAVIGATION_MENU },
	//{ "NAVIGATION_UP", VK_NAVIGATION_UP },
	//{ "NAVIGATION_DOWN", VK_NAVIGATION_DOWN },
	//{ "NAVIGATION_LEFT", VK_NAVIGATION_LEFT },
	//{ "NAVIGATION_RIGHT", VK_NAVIGATION_RIGHT },
	//{ "NAVIGATION_ACCEPT", VK_NAVIGATION_ACCEPT },
	//{ "NAVIGATION_CANCEL", VK_NAVIGATION_CANCEL },

	{ "NUMLOCK", VK_NUMLOCK },
	{ "SCROLLLOCK", VK_SCROLL },


	{ "LSHIFT", VK_LSHIFT },
	{ "RSHIFT", VK_RSHIFT },
	{ "LCTRL", VK_LCONTROL },
	{ "RCTRL", VK_RCONTROL },
	{ "LALT", VK_LMENU },
	{ "RALT", VK_RMENU },

	//{ "BROWSER_BACK", VK_BROWSER_BACK },
	//{ "BROWSER_FORWARD", VK_BROWSER_FORWARD },
	//{ "BROWSER_REFRESH", VK_BROWSER_REFRESH },
	//{ "BROWSER_STOP", VK_BROWSER_STOP },
	//{ "BROWSER_SEARCH", VK_BROWSER_SEARCH },
	//{ "BROWSER_FAVORITES", VK_BROWSER_FAVORITES },
	//{ "BROWSER_HOME", VK_BROWSER_HOME },

	//{ "VOLUME_MUTE", VK_VOLUME_MUTE },
	//{ "VOLUME_DOWN", VK_VOLUME_DOWN },
	//{ "VOLUME_UP", VK_VOLUME_UP },
	//{ "MEDIA_NEXT_TRACK", VK_MEDIA_NEXT_TRACK },
	//{ "MEDIA_PREV_TRACK", VK_MEDIA_PREV_TRACK },
	//{ "MEDIA_STOP", VK_MEDIA_STOP },
	//{ "MEDIA_PLAY_PAUSE", VK_MEDIA_PLAY_PAUSE },
	//{ "LAUNCH_MAIL", VK_LAUNCH_MAIL },
	//{ "LAUNCH_MEDIA_SELECT", VK_LAUNCH_MEDIA_SELECT },
	//{ "LAUNCH_APP1", VK_LAUNCH_APP1 },
	//{ "LAUNCH_APP2", VK_LAUNCH_APP2 },

	//{ "OEM_1", VK_OEM_1 },
	//{ "OEM_PLUS", VK_OEM_PLUS },
	//{ "OEM_COMMA", VK_OEM_COMMA },
	//{ "OEM_MINUS", VK_OEM_MINUS },
	//{ "OEM_PERIOD", VK_OEM_PERIOD },
	//{ "OEM_2", VK_OEM_2 },
	//{ "OEM_3", VK_OEM_3 },

	//{ "GAMEPAD_A", VK_GAMEPAD_A },
	//{ "GAMEPAD_B", VK_GAMEPAD_B },
	//{ "GAMEPAD_X", VK_GAMEPAD_X },
	//{ "GAMEPAD_Y", VK_GAMEPAD_Y },
	//{ "GAMEPAD_RIGHT_SHOULDER", VK_GAMEPAD_RIGHT_SHOULDER },
	//{ "GAMEPAD_LEFT_SHOULDER", VK_GAMEPAD_LEFT_SHOULDER },
	//{ "GAMEPAD_LEFT_TRIGGER", VK_GAMEPAD_LEFT_TRIGGER },
	//{ "GAMEPAD_RIGHT_TRIGGER", VK_GAMEPAD_RIGHT_TRIGGER },
	//{ "GAMEPAD_DPAD_UP", VK_GAMEPAD_DPAD_UP },
	//{ "GAMEPAD_DPAD_DOWN", VK_GAMEPAD_DPAD_DOWN },
	//{ "GAMEPAD_DPAD_LEFT", VK_GAMEPAD_DPAD_LEFT },
	//{ "GAMEPAD_DPAD_RIGHT", VK_GAMEPAD_DPAD_RIGHT },
	//{ "GAMEPAD_MENU", VK_GAMEPAD_MENU },
	//{ "GAMEPAD_VIEW", VK_GAMEPAD_VIEW },
	//{ "GAMEPAD_LEFT_THUMBSTICK_BUTTON", VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON },
	//{ "GAMEPAD_RIGHT_THUMBSTICK_BUTTON", VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON },
	//{ "GAMEPAD_LEFT_THUMBSTICK_UP", VK_GAMEPAD_LEFT_THUMBSTICK_UP },
	//{ "GAMEPAD_LEFT_THUMBSTICK_DOWN", VK_GAMEPAD_LEFT_THUMBSTICK_DOWN },
	//{ "GAMEPAD_LEFT_THUMBSTICK_RIGHT", VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT },
	//{ "GAMEPAD_LEFT_THUMBSTICK_LEFT", VK_GAMEPAD_LEFT_THUMBSTICK_LEFT },
	//{ "GAMEPAD_RIGHT_THUMBSTICK_UP", VK_GAMEPAD_RIGHT_THUMBSTICK_UP },
	//{ "GAMEPAD_RIGHT_THUMBSTICK_DOWN", VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN },
	//{ "GAMEPAD_RIGHT_THUMBSTICK_RIGHT", VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT },
	//{ "GAMEPAD_RIGHT_THUMBSTICK_LEFT", VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT }

};