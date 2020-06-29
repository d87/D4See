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

int old_xPos = 0;
int old_yPos = 0;

InputHandler::InputHandler(){
	//this->hWnd = hWnd;
	memset(KBBINDTABLE,0,sizeof(BINDING)*2048);
	memset(MBINDTABLE,0,sizeof(BINDING)*1024);
	memset(MOUSEMOVE,0,sizeof(BINDING));
	memset(&dragData,0,sizeof(DRAGDATA));
	MODIFIERMASK = 0;
	MButtonsState = 0;
}

void InputHandler::__MouseButton(unsigned char btnbit, int released){
	if (!released)	MButtonsState|=btnbit; //0|btnbit; //resetting 
	else			MButtonsState&=~btnbit;

	short bindaddr = (MODIFIERMASK << 8) + MButtonsState|btnbit;
	if (MBINDTABLE[bindaddr].Callback)
		MBINDTABLE[bindaddr].Callback(released,  0);
	MButtonsState&=~SINPUT_WHEEL;
}

int
InputHandler::ProcessInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
	switch(message){
		case WM_KEYDOWN:
		case WM_KEYUP:{
			short VKey = (short)wParam;
			switch (VKey){
				case VK_MENU:
				case VK_SHIFT:
				case VK_CONTROL:{
					unsigned char flag = SINPUT_CTRL;
					if (VKey == VK_MENU) flag = SINPUT_ALT;
					else if (VKey == VK_SHIFT) flag = SINPUT_SHIFT;

					if (message == WM_KEYDOWN)
						 MODIFIERMASK |=flag;
					else MODIFIERMASK &=~flag;

					short bindaddr = VKey;
					if (KBBINDTABLE[bindaddr].Callback)
						KBBINDTABLE[bindaddr].Callback( (message == WM_KEYUP) ? 1 : 0, 0);
					break;
				}

				default:{
					short bindaddr = (MODIFIERMASK << 8) + VKey;
					if (KBBINDTABLE[bindaddr].Callback)
						KBBINDTABLE[bindaddr].Callback( (message == WM_KEYUP) ? 1 : 0, 0);
				}
			}
			break;
		}
	
		case WM_MOUSEMOVE:{
			int new_xPos = GET_X_LPARAM(lParam);
			int new_yPos = GET_Y_LPARAM(lParam);
			//POINT * newPos = MAKEPOINTS(lParam);

			int dX = new_xPos - old_xPos;
			int dY = new_yPos - old_yPos;
			old_xPos = new_xPos;
			old_yPos = new_yPos;

			if (MOUSEMOVE[0].Callback)
				MOUSEMOVE[0].Callback(dX,dY);
				//(new_xPos,new_yPos);
			break;
			}
		case WM_LBUTTONDOWN:
			__MouseButton(SINPUT_BUTTON1, 0); break;
		case WM_LBUTTONUP:
			__MouseButton(SINPUT_BUTTON1, 1); break;
		case WM_RBUTTONDOWN:
			__MouseButton(SINPUT_BUTTON2, 0); break;
		case WM_RBUTTONUP:
			__MouseButton(SINPUT_BUTTON2, 1); break;
		case WM_MBUTTONDOWN:
			__MouseButton(SINPUT_BUTTON3, 0); break;
		case WM_MBUTTONUP:
			__MouseButton(SINPUT_BUTTON3, 1); break;
		case WM_XBUTTONDOWN:
			__MouseButton( (GET_XBUTTON_WPARAM (wParam) == 2) ? SINPUT_BUTTON5 : SINPUT_BUTTON4, 0); break;
		case WM_XBUTTONUP:
			__MouseButton( (GET_XBUTTON_WPARAM (wParam) == 2) ? SINPUT_BUTTON5 : SINPUT_BUTTON4, 1); break;
		case WM_MOUSEWHEEL:
			__MouseButton( SINPUT_WHEEL, GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? 1 : 0 ); break;

		default:
			return 0;
	
	}
	return 1;
}

inline int InputHandler::IsCtrlDown() {	return MODIFIERMASK&SINPUT_CTRL;  }
inline int InputHandler::IsAltDown()  {	return MODIFIERMASK&SINPUT_ALT;   }
inline int InputHandler::IsShiftDown(){	return MODIFIERMASK&SINPUT_SHIFT; }
inline int InputHandler::IsLMBDown()  { return MButtonsState&SINPUT_BUTTON1; }
inline int InputHandler::IsRMBDown()  { return MButtonsState&SINPUT_BUTTON2; }
inline int InputHandler::IsMMBDown()  { return MButtonsState&SINPUT_BUTTON3; }

void InputHandler::StartDragging(HWND hWnd){
	SetCapture(hWnd);
	GetCursorPos(&dragData.init);
}
void InputHandler::StopDragging(HWND hWnd){
	dragData.Xo = 0; dragData.Yo = 0;
	dragData.init.x = -1;
	ReleaseCapture();
}
int InputHandler::IsDragging(){
	if (dragData.init.x > 0) return 1;
	else return 0;
}


enum ModifierMask {
	CTRL = 1,
	ALT = 2,
	SHIFT = 4,
};

const std::unordered_map<std::wstring, ModifierMask> modifierMap = {
	{ L"CTRL", ModifierMask::CTRL },
	{ L"LCTRL", ModifierMask::CTRL },
	{ L"RCTRL", ModifierMask::CTRL },
	{ L"SHIFT", ModifierMask::SHIFT },
	{ L"LSHIFT", ModifierMask::SHIFT },
	{ L"RSHIFT", ModifierMask::SHIFT },
	{ L"ALT", ModifierMask::ALT },
	{ L"LALT", ModifierMask::ALT },
	{ L"RALT", ModifierMask::ALT },
};


int InputHandler::BindKey(std::wstring keyStr, void (*Callback)(int released, void*) ){
	uint8_t MODS = 0;
	
	std::wstring::iterator tokenStart = keyStr.begin();
	std::wstring::iterator it = keyStr.begin();
	std::wstring token;
	while(1) {
		if ( it == keyStr.end() || (*it) == L'-' ) {
			int off = tokenStart - keyStr.begin();
			int backtrack = (it == keyStr.end()) ? 0 : 1;
			int len = (it - backtrack) - tokenStart;
			token = keyStr.substr(off, len);

			auto search = modifierMap.find(token);
			if (search != modifierMap.end()) {
				ModifierMask mask = search->second;
				MODS |= mask;
			}
		}
		if (it == keyStr.end()) break;
		it++;
	}

	// Last token remains stored

	auto search = keyMap.find(token);
	if (search != keyMap.end()) {
		auto VKey = search->second;
		uint16_t bindaddr = (MODS << 8) + VKey;
		KBBINDTABLE[bindaddr].Callback = Callback;
		KBBINDTABLE[bindaddr].params = 0;
		return 1;
	}

	return 0;
}

int InputHandler::BindMouseButton(short Button, unsigned char MODS, void (*Callback)(int released, void*)){
	short bindaddr = (MODS << 8) + Button;
	MBINDTABLE[bindaddr].Callback = Callback;
	MBINDTABLE[bindaddr].params = 0;
	return 1;
}

int InputHandler::BindMouseMove(void (*Callback)(long,long)){
	MOUSEMOVE[0].Callback = Callback;
	return 1;
}


