#pragma once

#include <d2d1.h>
#pragma comment (lib,"d2d1.lib")
#include <dwrite.h>
#pragma comment (lib,"Dwrite.lib")

#include <stdint.h> // Standard types

#ifdef _DEBUG

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>


	#define LOG SPDLOG_DEBUG
	#define LOG_TRACE SPDLOG_TRACE
	#define LOG_DEBUG SPDLOG_DEBUG
	#define LOG_ERROR SPDLOG_ERROR
#else
	
	#define LOG(x)
	#define LOG_TRACE(x)
	#define LOG_DEBUG(x)
	#define LOG_ERROR(x)
#endif // DEBUG

#include "Resource.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define WM_FRAMEREADY WM_USER+1
#define WM_FRAMEERROR WM_USER+2


INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

extern ID2D1Factory* pD2DFactory;
//extern ID2D1DCRenderTarget* pRenderTarget;
extern ID2D1HwndRenderTarget* pRenderTarget;
extern IDWriteFactory* pDWriteFactory;
