#pragma once
#include <windows.h>
#include <windowsx.h>
#include <unordered_map>

namespace D4See {

	struct BINDING{
		void (*Callback)(int released, void*);
		void *params;
	};
	struct MMBINDING{
		void (*Callback)(long lastX, long lastY);
	};
	struct DRAGDATA{
		int Xo;
		int Yo;
		POINT init;
	};

	class InputHandler {
		unsigned char MODIFIERMASK;
		BINDING KBBINDTABLE[2048];
		BINDING MBINDTABLE[1024];
		MMBINDING MOUSEMOVE[1];
		unsigned char MButtonsState;
	
		public:
			DRAGDATA dragData;

		private:
			void __MouseButton(unsigned char btnbit, int released);

		public:
			InputHandler();
			int ProcessInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
			inline int IsCtrlDown();
			inline int IsAltDown();
			inline int IsShiftDown();
			inline int IsLMBDown();
			inline int IsRMBDown();
			inline int IsMMBDown();
			int BindKey(std::wstring keyStr, void (*Callback)(int released, void*));
			int BindMouseButton(short Button, unsigned char MODS, void (*Callback)(int released, void*));
			int BindMouseMove(void (*Callback)(long, long));

			void StartDragging(HWND hWnd);
			void StopDragging(HWND hWnd);
			int IsDragging();
	};
}

const std::unordered_map<std::wstring, uint8_t> keyMap = {
	// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
	{ L"MBUTTON1", VK_LBUTTON },
	{ L"MBUTTON2", VK_RBUTTON },
	{ L"CANCEL", VK_CANCEL },
	{ L"MBUTTON3", VK_MBUTTON },
	{ L"MBUTTON4", VK_XBUTTON1 },
	{ L"MBUTTON5", VK_XBUTTON2 },
	{ L"BACKSPACE", VK_BACK },
	{ L"TAB", VK_TAB },
	{ L"CLEAR", VK_CLEAR },
	{ L"RETURN", VK_RETURN },

	{ L"SHIFT", VK_SHIFT },
	{ L"CTRL", VK_CONTROL },
	{ L"ALT", VK_MENU },
	{ L"PAUSE", VK_PAUSE },
	{ L"CAPSLOCK", VK_CAPITAL },

	//{ L"KANA", VK_KANA },
	//{ L"HANGEUL", VK_HANGEUL },
	//{ L"HANGUL", VK_HANGUL },

	//{ L"JUNJA", VK_JUNJA },
	//{ L"FINAL", VK_FINAL },
	//{ L"HANJA", VK_HANJA },
	//{ L"KANJI", VK_KANJI },
	{ L"ESCAPE", VK_ESCAPE },
	//{ L"CONVERT", VK_CONVERT },
	//{ L"NONCONVERT", VK_NONCONVERT },
	//{ L"ACCEPT", VK_ACCEPT },
	//{ L"MODECHANGE", VK_MODECHANGE },

	{ L"SPACE", VK_SPACE },
	{ L"PAGEUP", VK_PRIOR },
	{ L"PAGEDOWN", VK_NEXT },
	{ L"END", VK_END },
	{ L"HOME", VK_HOME },
	{ L"LEFT", VK_LEFT },
	{ L"UP", VK_UP },
	{ L"RIGHT", VK_RIGHT },
	{ L"DOWN", VK_DOWN },
	//{ L"SELECT", VK_SELECT },
	//{ L"PRINT", VK_PRINT },
	//{ L"EXECUTE", VK_EXECUTE },
	{ L"PRINTSCREEN", VK_SNAPSHOT },
	{ L"INSERT", VK_INSERT },
	{ L"DELETE", VK_DELETE },
	{ L"HELP", VK_HELP },


	{ L"0", 0x30 },
	{ L"1", 0x31 },
	{ L"2", 0x32 },
	{ L"3", 0x33 },
	{ L"4", 0x34 },
	{ L"5", 0x35 },
	{ L"6", 0x36 },
	{ L"7", 0x37 },
	{ L"8", 0x38 },
	{ L"9", 0x39 },
	{ L"A", 0x41 },
	{ L"B", 0x42 },
	{ L"C", 0x43 },
	{ L"D", 0x44 },
	{ L"E", 0x45 },
	{ L"F", 0x46 },
	{ L"G", 0x47 },
	{ L"H", 0x48 },
	{ L"I", 0x49 },
	{ L"J", 0x4A },
	{ L"K", 0x4B },
	{ L"L", 0x4C },
	{ L"M", 0x4D },
	{ L"N", 0x4E },
	{ L"O", 0x4F },
	{ L"P", 0x50 },
	{ L"Q", 0x51 },
	{ L"R", 0x52 },
	{ L"S", 0x53 },
	{ L"T", 0x54 },
	{ L"U", 0x55 },
	{ L"V", 0x56 },
	{ L"W", 0x57 },
	{ L"X", 0x58 },
	{ L"Y", 0x59 },
	{ L"Z", 0x5A },


	{ L"LWIN", VK_LWIN },
	{ L"RWIN", VK_RWIN },
	//{ L"APPS", VK_APPS },

	{ L"SLEEP", VK_SLEEP },

	{ L"NUMPAD0", VK_NUMPAD0 },
	{ L"NUMPAD1", VK_NUMPAD1 },
	{ L"NUMPAD2", VK_NUMPAD2 },
	{ L"NUMPAD3", VK_NUMPAD3 },
	{ L"NUMPAD4", VK_NUMPAD4 },
	{ L"NUMPAD5", VK_NUMPAD5 },
	{ L"NUMPAD6", VK_NUMPAD6 },
	{ L"NUMPAD7", VK_NUMPAD7 },
	{ L"NUMPAD8", VK_NUMPAD8 },
	{ L"NUMPAD9", VK_NUMPAD9 },
	{ L"MULTIPLY", VK_MULTIPLY },
	{ L"ADD", VK_ADD },
	{ L"SEPARATOR", VK_SEPARATOR },
	{ L"SUBTRACT", VK_SUBTRACT },
	{ L"DECIMAL", VK_DECIMAL },
	{ L"DIVIDE", VK_DIVIDE },
	{ L"F1", VK_F1 },
	{ L"F2", VK_F2 },
	{ L"F3", VK_F3 },
	{ L"F4", VK_F4 },
	{ L"F5", VK_F5 },
	{ L"F6", VK_F6 },
	{ L"F7", VK_F7 },
	{ L"F8", VK_F8 },
	{ L"F9", VK_F9 },
	{ L"F10", VK_F10 },
	{ L"F11", VK_F11 },
	{ L"F12", VK_F12 },
	{ L"F13", VK_F13 },
	{ L"F14", VK_F14 },
	{ L"F15", VK_F15 },
	{ L"F16", VK_F16 },
	{ L"F17", VK_F17 },
	{ L"F18", VK_F18 },
	{ L"F19", VK_F19 },
	{ L"F20", VK_F20 },
	{ L"F21", VK_F21 },
	{ L"F22", VK_F22 },
	{ L"F23", VK_F23 },
	{ L"F24", VK_F24 },

	//{ L"NAVIGATION_VIEW", VK_NAVIGATION_VIEW },
	//{ L"NAVIGATION_MENU", VK_NAVIGATION_MENU },
	//{ L"NAVIGATION_UP", VK_NAVIGATION_UP },
	//{ L"NAVIGATION_DOWN", VK_NAVIGATION_DOWN },
	//{ L"NAVIGATION_LEFT", VK_NAVIGATION_LEFT },
	//{ L"NAVIGATION_RIGHT", VK_NAVIGATION_RIGHT },
	//{ L"NAVIGATION_ACCEPT", VK_NAVIGATION_ACCEPT },
	//{ L"NAVIGATION_CANCEL", VK_NAVIGATION_CANCEL },

	{ L"NUMLOCK", VK_NUMLOCK },
	{ L"SCROLLLOCK", VK_SCROLL },


	{ L"LSHIFT", VK_LSHIFT },
	{ L"RSHIFT", VK_RSHIFT },
	{ L"LCTRL", VK_LCONTROL },
	{ L"RCTRL", VK_RCONTROL },
	{ L"LALT", VK_LMENU },
	{ L"RALT", VK_RMENU },

	//{ L"BROWSER_BACK", VK_BROWSER_BACK },
	//{ L"BROWSER_FORWARD", VK_BROWSER_FORWARD },
	//{ L"BROWSER_REFRESH", VK_BROWSER_REFRESH },
	//{ L"BROWSER_STOP", VK_BROWSER_STOP },
	//{ L"BROWSER_SEARCH", VK_BROWSER_SEARCH },
	//{ L"BROWSER_FAVORITES", VK_BROWSER_FAVORITES },
	//{ L"BROWSER_HOME", VK_BROWSER_HOME },

	//{ L"VOLUME_MUTE", VK_VOLUME_MUTE },
	//{ L"VOLUME_DOWN", VK_VOLUME_DOWN },
	//{ L"VOLUME_UP", VK_VOLUME_UP },
	//{ L"MEDIA_NEXT_TRACK", VK_MEDIA_NEXT_TRACK },
	//{ L"MEDIA_PREV_TRACK", VK_MEDIA_PREV_TRACK },
	//{ L"MEDIA_STOP", VK_MEDIA_STOP },
	//{ L"MEDIA_PLAY_PAUSE", VK_MEDIA_PLAY_PAUSE },
	//{ L"LAUNCH_MAIL", VK_LAUNCH_MAIL },
	//{ L"LAUNCH_MEDIA_SELECT", VK_LAUNCH_MEDIA_SELECT },
	//{ L"LAUNCH_APP1", VK_LAUNCH_APP1 },
	//{ L"LAUNCH_APP2", VK_LAUNCH_APP2 },

	//{ L"OEM_1", VK_OEM_1 },
	//{ L"OEM_PLUS", VK_OEM_PLUS },
	//{ L"OEM_COMMA", VK_OEM_COMMA },
	//{ L"OEM_MINUS", VK_OEM_MINUS },
	//{ L"OEM_PERIOD", VK_OEM_PERIOD },
	//{ L"OEM_2", VK_OEM_2 },
	//{ L"OEM_3", VK_OEM_3 },

	//{ L"GAMEPAD_A", VK_GAMEPAD_A },
	//{ L"GAMEPAD_B", VK_GAMEPAD_B },
	//{ L"GAMEPAD_X", VK_GAMEPAD_X },
	//{ L"GAMEPAD_Y", VK_GAMEPAD_Y },
	//{ L"GAMEPAD_RIGHT_SHOULDER", VK_GAMEPAD_RIGHT_SHOULDER },
	//{ L"GAMEPAD_LEFT_SHOULDER", VK_GAMEPAD_LEFT_SHOULDER },
	//{ L"GAMEPAD_LEFT_TRIGGER", VK_GAMEPAD_LEFT_TRIGGER },
	//{ L"GAMEPAD_RIGHT_TRIGGER", VK_GAMEPAD_RIGHT_TRIGGER },
	//{ L"GAMEPAD_DPAD_UP", VK_GAMEPAD_DPAD_UP },
	//{ L"GAMEPAD_DPAD_DOWN", VK_GAMEPAD_DPAD_DOWN },
	//{ L"GAMEPAD_DPAD_LEFT", VK_GAMEPAD_DPAD_LEFT },
	//{ L"GAMEPAD_DPAD_RIGHT", VK_GAMEPAD_DPAD_RIGHT },
	//{ L"GAMEPAD_MENU", VK_GAMEPAD_MENU },
	//{ L"GAMEPAD_VIEW", VK_GAMEPAD_VIEW },
	//{ L"GAMEPAD_LEFT_THUMBSTICK_BUTTON", VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON },
	//{ L"GAMEPAD_RIGHT_THUMBSTICK_BUTTON", VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON },
	//{ L"GAMEPAD_LEFT_THUMBSTICK_UP", VK_GAMEPAD_LEFT_THUMBSTICK_UP },
	//{ L"GAMEPAD_LEFT_THUMBSTICK_DOWN", VK_GAMEPAD_LEFT_THUMBSTICK_DOWN },
	//{ L"GAMEPAD_LEFT_THUMBSTICK_RIGHT", VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT },
	//{ L"GAMEPAD_LEFT_THUMBSTICK_LEFT", VK_GAMEPAD_LEFT_THUMBSTICK_LEFT },
	//{ L"GAMEPAD_RIGHT_THUMBSTICK_UP", VK_GAMEPAD_RIGHT_THUMBSTICK_UP },
	//{ L"GAMEPAD_RIGHT_THUMBSTICK_DOWN", VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN },
	//{ L"GAMEPAD_RIGHT_THUMBSTICK_RIGHT", VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT },
	//{ L"GAMEPAD_RIGHT_THUMBSTICK_LEFT", VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT }

};