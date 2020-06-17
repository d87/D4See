#pragma once

#include <Windows.h>
#include <shlobj.h>
#include <iostream>
#include <filesystem>
#include <chrono>
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
    //int w_working_area; //screen size excluding taskbar
    //int h_working_area;
    int w_window;
    int h_window; // including borders and caption
    int w_client;
    int h_client;

    int x_origin;
    int y_origin;

    int w_border;
    int h_border;
    int h_caption;

    float scale = 1.3f;
    
    int x_poffset = 0;
    int y_poffset = 0;
    int w_scaled;
    int h_scaled;

    int mouseX = 0;
    int mouseY = 0;

private:
    std::chrono::system_clock::time_point lastGeneratedSizingEvent;
    WINDOW_SAVED_DATA stash;

public:
    void SelectFrame(MemoryFrame* f);
    void ResizeForImage();
    void ToggleFullscreen();
    void WriteOrigin();
    void ReadOrigin();
    void UpdateOrigin();
    bool WasGeneratingEvents();
    void Pan(int x, int y);
    bool SaveWindowParams();
    void GetCenteredImageRect(RECT* rc);
    void UpdateWindowSizeInfo();
    void _TouchSizeEventTimestamp();

    
};