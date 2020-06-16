#pragma once

#include <Windows.h>
#include "MemoryFrame.h"

struct WINDOW_SAVED_DATA {
	bool isMaximized;
	long style;
	long exstyle;
	RECT rc;
};

class WindowManager {
public:
    HWND hWnd;
    MemoryFrame * frame;
    bool newImagePending = false;
    bool fastDrawDone = false;
    bool isMovingOrSizing = false;
    bool isMaximized = false;
    bool isFullscreen = false;
    int w_working_area; //screen size excluding taskbar
    int h_working_area;
    int w_window;
    int h_window; // including borders and caption
    int w_client;
    int h_client;

    int w_border;
    int h_border;
    int h_caption;

    float scale = 1.0f;
    
    int x_poffset = 0;
    int y_poffset = 0;
    int w_scaled;
    int h_scaled;

    int mouseX = 0;
    int mouseY = 0;

    WINDOW_SAVED_DATA stash;


    void SelectFrame(MemoryFrame* f);
    void ResizeForImage();
    void ToggleFullscreen();
    void Pan(int x, int y);
    bool SaveWindowParams();
    void GetCenteredImageRect(RECT* rc);
    void GetWindowSize();

};