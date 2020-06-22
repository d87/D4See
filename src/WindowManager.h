#pragma once

#include <Windows.h>
#include <shlobj.h>
#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <chrono>
#include "ImageContainer.h"
#include "resource.h"
#include "util.h"
#include "Playlist.h"
#include "Animation.h"
#include "D4See.h"

#include "libs/toml11/toml.hpp"

#define BOOLCOMMANDCHECK(x) ( (x) ? MF_CHECKED : MF_UNCHECKED )
#define BOOLCOMMANDENABLE(x) ( (x) ? MF_ENABLED: MF_GRAYED )

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
    ImageContainer* frame = nullptr;
    ImageContainer* frame_prefetch = nullptr; // prefecth container
    Playlist* playlist = nullptr;
    PlaylistSortMethod sortMethod = PlaylistSortMethod::ByName;
    //bool newImagePending = false;

    bool isMovingOrSizing = false;
    bool isPanning = false;
    bool isMaximized = false;
    bool isFullscreen = false;
    bool isBorderless = false;
    bool isAlwaysOnTop = false;

    int w_client; // client area, everything inside the window frame
    int h_client;

    int x_origin; // point on the screen around which the windows is constucted
    int y_origin;

    //int w_border; // Window metrics
    //int h_border;
    //int h_caption;

    //int borderlessBorder = 0;
    bool zoomLock = false;
    bool stretchToScreenWidth = true;
    bool stretchToScreenHeight = false;
    bool shrinkToScreenWidth = false;
    bool shrinkToScreenHeight = false;


    float scale_manual = 1.0f; // 
    float scale_effective = 1.0f; // Actual current scale
    
    float x_poffset = 0; // Panning offset
    float y_poffset = 0;

    int w_scaled; // Dimensions after applying scale
    int h_scaled;

    int mouseX = 0;  // Used to store previous mouse position when panning
    int mouseY = 0;

    std::unordered_map<std::string, Animation*> animations{};

private:
    std::chrono::system_clock::time_point lastGeneratedSizingEvent; // (generated not by user)
    WINDOW_SAVED_DATA stash;

public:
    void Redraw(unsigned int addFlags = 0);
    void StopTimer(UINT_PTR id);
    void NextImage();
    void PreviousImage();
    void LoadImageFromPlaylist(int prefetchDir);
    void StartPrefetch(ImageContainer* f);
    void ShowPrefetch();
    void DiscardPrefetch();
    void SelectImage(ImageContainer* f);
    void SelectPlaylist(Playlist* playlist);
    void ResizeForImage();
    void ManualZoom(float mod, float absolute = 0.0);
    void ManualZoomToPoint(float mod, int dstx, int dsty);
    void ToggleAlwaysOnTop();
    void ToggleFullscreen();
    void ToggleBorderless(int doRedraw = 1);
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