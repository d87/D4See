#pragma once

#include <Windows.h>
#include <shlobj.h>
#include <iostream>
#include <filesystem>
#include <chrono>
#include "MemoryFrame.h"
#include "resource.h"
#include "util.h"
#include "Playlist.h"

#include "libs/toml11/toml.hpp"

#define BOOLCOMMANDCHECK(x) ( (x) ? MF_CHECKED : MF_UNCHECKED )

#define D4S_TIMER_HQREDRAW 1001
#define D4S_PREFETCH_TIMEOUT 1003


struct WINDOW_SAVED_DATA {
	bool isMaximized;
	long style;
	long exstyle;
	RECT rc;
};

class WindowManager {
public:
    HWND hWnd;
    MemoryFrame* frame = nullptr;
    MemoryFrame* frame2 = nullptr;
    Playlist* playlist = nullptr;
    //bool newImagePending = false;

    bool fastDrawDone = false;
    bool isMovingOrSizing = false;
    bool isMaximized = false;
    bool isFullscreen = false;
    bool isBorderless = false;
    bool alwaysOnTop = false;

    int w_client;
    int h_client;

    int x_origin;
    int y_origin;

    int w_border;
    int h_border;
    int h_caption;

    bool zoomLock = false;
    bool stretchToScreenWidth = true;
    bool stretchToScreenHeight = false;
    bool shrinkToScreenWidth = false;
    bool shrinkToScreenHeight = false;


    float scale_manual = 1.0f;
    float scale_effective = 1.0f;
    
    int x_poffset = 0;
    int y_poffset = 0;
    int w_scaled;
    int h_scaled;

    int mouseX = 0;
    int mouseY = 0;

private:
    std::chrono::system_clock::time_point lastGeneratedSizingEvent;
    WINDOW_SAVED_DATA stash;
    WINDOW_SAVED_DATA borderlessStash;

public:
    void Redraw(unsigned int addFlags = 0);
    void ScheduleRedraw(unsigned int ms);
    void StopTimer(UINT_PTR id);
    void NextImage();
    void PreviousImage();
    void StartPrefetch(MemoryFrame* f);
    void ShowPrefetch();
    void DiscardPrefetch();
    void SelectFrame(MemoryFrame* f);
    void SelectPlaylist(Playlist* playlist);
    void ResizeForImage(bool HQRedraw = false);
    void ManualZoom(float mod, float absolute = 0.0);
    void ToggleFullscreen();
    void ToggleBorderless();
    void WriteOrigin();
    void ReadOrigin();

    toml::value ReadConfig();
    void RestoreConfigValues(toml::value& data);
    void DumpConfigValues(toml::value& data);
    void WriteConfig(toml::value& data);

    void UpdateOrigin();
    bool WasGeneratingEvents();
    void LimitPanOffset();
    void Pan(int x, int y);
    bool SaveWindowParams(WINDOW_SAVED_DATA* cont);
    void GetCenteredImageRect(RECT* rc);
    void ShowPopupMenu(POINT &p);
    void HandleMenuCommand(unsigned int uIDItem);
    void UpdateWindowSizeInfo();
    void _TouchSizeEventTimestamp();
    ~WindowManager();
    
};