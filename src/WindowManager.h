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
#include "Canvas.h"
#include "D4See.h"
#include "Input.h"
#include "Configuration.h"

#include "libs/toml11/toml.hpp"

#define BOOLCOMMANDCHECK(x) ( (x) ? MF_CHECKED : MF_UNCHECKED )
#define BOOLCOMMANDENABLE(x) ( (x) ? MF_ENABLED: MF_GRAYED )

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
    std::shared_ptr<ImageContainer> frame = nullptr;
    std::shared_ptr<ImageContainer> frame_prefetch = nullptr; // prefecth container
    Playlist* playlist = nullptr;
    PlaylistSortMethod sortMethod = PlaylistSortMethod::ByName;
    D4See::InputHandler input;

    bool isMovingOrSizing = false;
    bool isPanning = false;
    bool isMaximized = false;
    bool isFullscreen = false;
    bool isBorderless = false;
    bool isAlwaysOnTop = false;


    int x_origin; // point on the screen around which the window is constucted
    int y_origin;

    //enum {
    //    WND_XSCREENSTRETCH = 1,
    //    WND_YSCREENSTRETCH = 2,
    //    WND_XSCREENSHRINK = 4,
    //    WND_YSCREENSHRINK = 8,
    //};
    //uint8_t stretchState = 0;
    bool zoomLock = false;
    bool stretchToScreenWidth = true;
    bool stretchToScreenHeight = false;
    bool shrinkToScreenWidth = false;
    bool shrinkToScreenHeight = false;

    D4See::Canvas canvas;

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
    void StartPrefetch(std::shared_ptr<ImageContainer> f);
    void ShowPrefetch();
    void LimitPanOffset();
    void DiscardPrefetch();
    void SelectImage(std::shared_ptr<ImageContainer> f);
    void SelectPlaylist(Playlist* playlist);
    void GetWindowSizeForImage(RECT& rrc);
    void ResizeForImage();
    void ManualZoom(float mod, float absolute = 0.0);
    void ManualZoomToPoint(float mod, float absolute, int dstx, int dsty);
    void ToggleAlwaysOnTop();
    void ToggleFullscreen();
    void ToggleBorderless(int doRedraw = 1);
    void WriteOrigin();
    void ReadOrigin();

    void RestoreConfigValues(D4See::Configuration& config);

    void UpdateOrigin();
    bool WasGeneratingEvents();
    void Pan(int x, int y);
    bool SaveWindowParams(WINDOW_SAVED_DATA* cont);
    void GetCenteredImageRect(RECT* rc);
    void ShowPopupMenu(POINT &p);
    void HandleMenuCommand(unsigned int uIDItem);
    void UpdateWindowSizeInfo();
    void _TouchSizeEventTimestamp();
    ~WindowManager();
    
};

extern WindowManager gWinMgr;