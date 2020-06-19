#pragma once

#include "Resource.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define WM_FRAMEREADY WM_USER+1

INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

extern ID2D1Factory* pD2DFactory;
extern ID2D1DCRenderTarget* pRenderTarget;