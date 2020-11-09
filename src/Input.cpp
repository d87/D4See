#include "Input.h"
#include <stdio.h>
#include <stdint.h>
#include <sstream>
#include <iterator>

using namespace D4See;

const uint8_t SINPUT_CTRL = 1;
const uint8_t SINPUT_ALT = 2;
const uint8_t SINPUT_SHIFT = 4;
const uint8_t SINPUT_BUTTON1 = 1;
const uint8_t SINPUT_BUTTON2 = 2;
const uint8_t SINPUT_BUTTON3 = 4;
const uint8_t SINPUT_BUTTON4 = 8;
const uint8_t SINPUT_BUTTON5 = 16;
const uint8_t SINPUT_WHEEL = 32;

InputHandler::InputHandler(){
	memset(kbBindTable,0,sizeof(Action)*2048);
}


void InputHandler::FireAction(uint8_t VKey, int isDown) {
	uint8_t modMask = GetModifierMask();
	uint16_t bindaddr = (modMask << 8) + VKey;

	if (isDown) {
		if ((int)kbBindTable[bindaddr].actionType) {
			Action action = kbBindTable[bindaddr];

			// KeyDown event is sent repeatedly, so avoiding pushing duplicates
			bool found = false;
			for (int i = 0; i < pressedActions.size(); i++) {
				if (pressedActions[i].VKey == VKey && pressedActions[i].modMask == modMask) {
					found = true;
				}
			}
			if (!found) {
				ActiveKeyPress keyPress;
				keyPress.modMask = modMask;
				keyPress.VKey = VKey;
				keyPress.action = action;
				pressedActions.push_back(keyPress);
			}

			action.cbOnDown();
			if (action.cbOnMouseMove != NULL) {
				currentMouseMoveCallback = action.cbOnMouseMove;
			}
		}
	}
	else {
		// When releasing it doesn't care whatever the modifier mask was
		for (int i = 0; i < pressedActions.size(); i++) {
			if (pressedActions[i].VKey == VKey) {

				Action& action = pressedActions[i].action;
				if (action.cbOnUp != NULL) {
					action.cbOnUp();
					currentMouseMoveCallback = NULL;
				}

				pressedActions.erase(pressedActions.begin()+i);
				i--;
			}
		}
	}
}

int
InputHandler::ProcessInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
	switch(message){
		case WM_SYSKEYDOWN: // For Alt handling, which is very weird on Windows
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:{
			short VKey = (short)wParam;
			switch (VKey){

				default:{
					FireAction(VKey, (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) ? 1 : 0);
				}
			}
			break;
		}
	
		case WM_MOUSEMOVE:{
			if (currentMouseMoveCallback != NULL) {
				currentMouseMoveCallback();
			}
			//for (int i = 0; i < pressedActions.size(); i++) {
			//	auto cbOnMouseMove = pressedActions[i].action.cbOnMouseMove;
			//	if (cbOnMouseMove != NULL) {
			//		cbOnMouseMove();
			//	}
			//}
			break;
		}
		case WM_LBUTTONDOWN:
			FireAction(MBVKeys::MBUTTON1, 1); break;
		case WM_LBUTTONUP:
			FireAction(MBVKeys::MBUTTON1, 0); break;
		case WM_RBUTTONDOWN:
			FireAction(MBVKeys::MBUTTON2, 1); break;
		case WM_RBUTTONUP:
			FireAction(MBVKeys::MBUTTON2, 0); break;
		case WM_MBUTTONDOWN:
			FireAction(MBVKeys::MBUTTON3, 1); break;
		case WM_MBUTTONUP:
			FireAction(MBVKeys::MBUTTON3, 0); break;
		case WM_XBUTTONDOWN: {
			MBVKeys btn = ((GET_XBUTTON_WPARAM(wParam) == 2) ? MBVKeys::MBUTTON5 : MBVKeys::MBUTTON4);
			FireAction(btn, 1);
			break;
		}
		case WM_XBUTTONUP: {
			MBVKeys btn = ((GET_XBUTTON_WPARAM(wParam) == 2) ? MBVKeys::MBUTTON5 : MBVKeys::MBUTTON4);
			FireAction(btn, 0);
			break;
		}
		case WM_LBUTTONDBLCLK:
			FireAction(MBVKeys::MBUTTON1_DBL, 1); break;
		case WM_RBUTTONDBLCLK:
			FireAction(MBVKeys::MBUTTON2_DBL, 1); break;

		case WM_MOUSEWHEEL: {
			int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			if (zDelta < 0) {
				FireAction(MBVKeys::MWHEELDOWN, 1);
			}
			else if (zDelta > 0) {
				FireAction(MBVKeys::MWHEELUP, 1);
			}
			break;
		}

		default:
			return 0;
	
	}
	return 1;
}



enum ModifierMask {
	CTRL = 1,
	ALT = 2,
	SHIFT = 4,
	WIN = 8
};

uint8_t
InputHandler::GetModifierMask() {
	uint8_t mask = 0;
	mask |= (GetAsyncKeyState(VK_CONTROL) ? ModifierMask::CTRL: 0);
	mask |= (GetAsyncKeyState(VK_LMENU) ? ModifierMask::ALT: 0); // VK_MENU is bugged
	mask |= (GetAsyncKeyState(VK_SHIFT) ? ModifierMask::SHIFT: 0);
	return mask;
}

const std::unordered_map<std::string, ModifierMask> modifierMap = {
	{ "CTRL", ModifierMask::CTRL },
	{ "LCTRL", ModifierMask::CTRL },
	{ "RCTRL", ModifierMask::CTRL },
	{ "SHIFT", ModifierMask::SHIFT },
	{ "LSHIFT", ModifierMask::SHIFT },
	{ "RSHIFT", ModifierMask::SHIFT },
	{ "ALT", ModifierMask::ALT },
	{ "LALT", ModifierMask::ALT },
	{ "RALT", ModifierMask::ALT },
	//{ "WIN", ModifierMask::WIN },
	//{ "LWIN", ModifierMask::WIN },
	//{ "RWIN", ModifierMask::WIN },
};


int InputHandler::BindKey(std::string& keyStr, const std::string& actionStr){
	uint8_t modMask = 0;
	
	std::string::iterator tokenStart = keyStr.begin();
	std::string::iterator it = keyStr.begin();
	std::string token;
	while(1) {
		if ( it == keyStr.end() || (*it) == '-' ) {
			int off = tokenStart - keyStr.begin();
			int len = it - tokenStart;
			token = keyStr.substr(off, len);

			int skip = (it == keyStr.end()) ? 0 : 1;
			tokenStart = it+skip;

			auto search = modifierMap.find(token);
			if (search != modifierMap.end()) {
				ModifierMask mask = search->second;
				modMask |= mask;
			}
		}
		if (it == keyStr.end()) break;
		it++;
	}

	// Last token remains stored

	auto search = keyMap.find(token);
	if (search != keyMap.end()) {
		auto VKey = search->second;
		uint16_t bindaddr = (modMask << 8) + VKey;

		auto actionMap = getActionMap();
		auto actionSearch = actionMap.find(actionStr);
		if (actionSearch != actionMap.end()) {
			kbBindTable[bindaddr] = actionSearch->second;
			return 1;
		}
	}

	return 0;
}
