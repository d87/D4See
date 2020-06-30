#pragma once

#include <Windows.h>
#include <unordered_map>
#include <string>

enum class ActionType {
	PRESS = 0b00001, // a single handler on either down or up
	HOLD = 0b00010, // uses both down and up handlers
	MOUSEMOVE = 0b00100, // both + mouse move handler
	MOUSESRC = 0b01000,
	KEYBOARDSRC = 0b10000,
	ANYSRC = 0b11000,
};

typedef void (*callback_function)(void);

//typedef int callback_function;

struct Action {
	ActionType actionType;
	//Action* action, void* params
	callback_function cbOnDown;
	callback_function cbOnUp;
	callback_function cbOnMouseMove;
};

std::unordered_map<std::string, Action>& getActionMap();


//void RegisterSinglePressAction(std::string actionName, callback_function cbOnClick);
//void RegisterHoldAction(std::string actionName, callback_function cbOnDown, callback_function cbOnUp);
//void RegisterMouseMoveAction(std::string actionName, callback_function cbOnDown, callback_function cbOnUp, callback_function cbOnMouseMove);
void RegisterActions();