#include "WindowManager.h"

namespace fs = std::filesystem;

void WindowManager::Redraw(unsigned int addFlags) {
    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | addFlags);
    LOG_TRACE("--x Invalidated");
}

void WindowManager::StopTimer(UINT_PTR id) {
    KillTimer(hWnd, id);
}

//void ClearWindowForFrame(HWND hWnd, ImageContainer* f) {
//
//    //HBRUSH newBrush = CreateSolidBrush(RGB(80, 80, 80));
//    //FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
//    //FillRect(hdc, &rc, newBrush);
//    //DeleteObject(newBrush);
//    gWinMgr.ResizeForImage();
//
//
//    //GdiFlush();
//
//    // It's a bit better with ERASENOW, but it barely matters
//    //RedrawWindow(hWnd, NULL, NULL, RDW_UPDATENOW|RDW_INVALIDATE);
//    RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
//    //RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
//}

void WindowManager::ShowPrefetch() {
    LOG_DEBUG("Switching to prefetched image");
    SelectImage(frame_prefetch);
    frame_prefetch = nullptr;
    //frame->drawId = frame->decoderBatchId;
    //gWinMgr.newImagePending = true;

    //ClearWindowForFrame(hWnd, frame);

    using namespace std::chrono_literals;
    auto status = frame->threadInitFinished.wait_for(2ms);
    if (status == std::future_status::ready) {
        LOG_TRACE("Decoder thread initialized");
        ResizeForImage();
    }
    Redraw(RDW_ERASE);
}

void WindowManager::DiscardPrefetch() {
    //if (frame_prefetch)
        //delete frame_prefetch;
    frame_prefetch = nullptr;
}

void WindowManager::StartPrefetch(std::shared_ptr<ImageContainer> f) {
    frame_prefetch = f;
    // Start 2 minute timer
    SetTimer(hWnd, D4S_PREFETCH_TIMEOUT, 120000, NULL);
}

void WindowManager::PreviousImage() {
    if (playlist.Move(PlaylistPos::Current, -1)) {
        LoadImageFromPlaylist(-1);
    }
}

void WindowManager::NextImage() {
    if (playlist.Move(PlaylistPos::Current, +1)) {
        LoadImageFromPlaylist(+1);
    }
}

void WindowManager::LoadImageFromPlaylist(int prefetchDir) {
    PlaylistEntry* cur = playlist.Current();
    PlaylistEntry* following = nullptr;
    if (prefetchDir > 0)
        following = playlist.Next();
    else if (prefetchDir < 0)
        following = playlist.Prev();

    if (cur) {
        bool prefetchHit = false;
        if (frame_prefetch)
            if (frame_prefetch->filename == cur->path)
                prefetchHit = true;
        if (prefetchHit) {
            ShowPrefetch();
        }
        else { // Changed direction or jumped more than 1
            DiscardPrefetch();
            SelectImage(std::make_shared<ImageContainer>(hWnd, cur->path, cur->format));
        }
    }
    if (following) {
        StartPrefetch(std::make_shared<ImageContainer>(hWnd, following->path, following->format));
    }
    else {
        frame_prefetch = nullptr;
    }
}


void WindowManager::LimitPanOffset() {
    float x_limit = static_cast<float>(canvas.w_scaled - canvas.w_client);
    canvas.x_poffset = std::min(canvas.x_poffset, x_limit);
    canvas.x_poffset = std::max(canvas.x_poffset, 0.0f);

    float y_limit = static_cast<float>(canvas.h_scaled - canvas.h_client);
    canvas.y_poffset = std::min(canvas.y_poffset, y_limit);
    canvas.y_poffset = std::max(canvas.y_poffset, 0.0f);
}


void WindowManager::Pan(int x, int y) {
    canvas.x_poffset += x;
    canvas.y_poffset += y;
    LimitPanOffset();

    Redraw();
}

void WindowManager::UpdateWindowSizeInfo() {
    // Used only to update client area dimensions after WM_SIZE to limit panning offset
    RECT crc;
    GetClientRect(hWnd, &crc);
    canvas.h_client = crc.bottom - crc.top;
    canvas.w_client = crc.right - crc.left;
}

void WindowManager::GetCenteredImageRect(RECT* rc) {
    RECT crc;
    GetClientRect(hWnd, &crc);
    //h_client = crc.bottom - crc.top;
    //w_client = crc.right - crc.left;

    int ch = crc.bottom - crc.top;
    int cw = crc.right - crc.left;

    int x = 0;
    int y = 0;
    if (cw > canvas.w_scaled) {
        x = (cw - canvas.w_scaled) / 2;
    }

    if (ch > canvas.h_scaled) {
        y = (ch - canvas.h_scaled) / 2;
    }
    rc->left = x;
    rc->top = y;
    rc->right = x + canvas.w_scaled;
    rc->bottom = y + canvas.h_scaled;
}

bool WindowManager::SaveWindowParams(WINDOW_SAVED_DATA*cont) {
    long style = GetWindowLong(hWnd, GWL_STYLE);
    cont->isMaximized = style & WS_MAXIMIZE;

    if (cont->isMaximized) {
        //SendMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
        return cont->isMaximized;
    }

    cont->style = GetWindowLong(hWnd, GWL_STYLE);
    cont->exstyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    GetWindowRect(hWnd, &cont->rc);
    return cont->isMaximized;
}

void WindowManager::ToggleAlwaysOnTop() {
    DWORD dwExStyle = (DWORD)GetWindowLong(hWnd, GWL_EXSTYLE);
    //bool isAOT = ((dwExStyle & WS_EX_TOPMOST) != 0);
    isAlwaysOnTop = !isAlwaysOnTop;

    SetWindowPos(hWnd, isAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void WindowManager::ToggleFullscreen() {
    if (!isFullscreen) {
        bool isCurrentlyFullscreen = SaveWindowParams(&this->stash);

    }

    isFullscreen = !isFullscreen;

    if (isFullscreen) {
        // Set new window style and size.
        SetWindowLong(hWnd, GWL_STYLE, stash.style & ~(WS_CAPTION | WS_THICKFRAME));
        SetWindowLong(hWnd, GWL_EXSTYLE, stash.exstyle & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));

        // On expand, if we're given a window_rect, grow to it, otherwise do
        // not resize.
        MONITORINFO monitor_info;
        monitor_info.cbSize = sizeof(monitor_info);
        GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &monitor_info);
        RECT* rc = &monitor_info.rcMonitor;
        int mw = rc->right - rc->left;
        int mh = rc->bottom - rc->top;
        SetWindowPos(hWnd, NULL, rc->left, rc->top, mw, mh, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
    else {
        // Reset original window style and size.  The multiple window size/moves
        // here are ugly, but if SetWindowPos() doesn't redraw, the taskbar won't be
        // repainted.  Better-looking methods welcome.
        SetWindowLong(hWnd, GWL_STYLE, stash.style);
        SetWindowLong(hWnd, GWL_EXSTYLE, stash.exstyle);

        // On restore, resize to the previous saved rect size.
        //RECT* rc = &stash.rc;
        //int w = rc->right - rc->left;
        //int h = rc->bottom - rc->top;
        //SetWindowPos(hWnd, NULL, rc->left, rc->top, w, h, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

        if (stash.isMaximized)
            ::SendMessage(hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
    }
    ResizeForImage();
    Redraw();
}

void WindowManager::ToggleBorderless(int doRedraw) {
    if (isFullscreen) return;

    isBorderless = !isBorderless;

    int style = GetWindowLong(hWnd, GWL_STYLE);
    int exstyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    if (isBorderless) {
        SetWindowLong(hWnd, GWL_STYLE, style & ~(WS_CAPTION | WS_THICKFRAME));
        //SetWindowLong(hWnd, GWL_EXSTYLE, exstyle & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
    } else {
        //SetWindowLong(hWnd, GWL_STYLE, borderlessStash.style);
        SetWindowLong(hWnd, GWL_STYLE, style | WS_CAPTION | WS_THICKFRAME);
        //SetWindowLong(hWnd, GWL_EXSTYLE, borderlessStash.exstyle);
    }
    //SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOREPOSITION | SWP_NOREDRAW);
    if (doRedraw) {
        ResizeForImage();
        Redraw();
    }
}

void WindowManager::SelectImage(std::shared_ptr<ImageContainer> f) {
    LOG_DEBUG("<<<<<<< FRAME SWITCH >>>>>>>");
    //LOG_DEBUG(" -- {0} --", f->filename);
    frame = f;
    canvas.x_poffset = 0;
    canvas.y_poffset = 0;
    animations.clear();
    if (!zoomLock) {
        canvas.scale_manual = 1.0f;
    }
    std::wstring title = playlist.Current()->filename + L" - D4See";
    SetWindowTextW(hWnd, title.c_str());
}


void WindowManager::_TouchSizeEventTimestamp() {
    lastGeneratedSizingEvent = std::chrono::system_clock::now();
}

bool WindowManager::WasGeneratingEvents() {
    using namespace std::chrono_literals;

    auto now = std::chrono::system_clock::now();
    auto dt = now - lastGeneratedSizingEvent;

    return dt < 100ms;
}


void WindowManager::RestoreConfigValues(D4See::Configuration& config) {
    if (config.isBorderless) {
        ToggleBorderless(0);
    }
    sortMethod = config.sortMethod;
    stretchToScreenWidth = config.stretchToScreenWidth;
    stretchToScreenHeight = config.stretchToScreenHeight;
    shrinkToScreenWidth = config.shrinkToScreenWidth;
    shrinkToScreenHeight = config.shrinkToScreenHeight;
}
/*
void WindowManager::DumpConfigValues(toml::value& data) {

    std::string sortStr = (sortMethod == PlaylistSortMethod::ByDateModified) ? "ByDateModified" : "ByName";

    data["general"]["SortMethod"] = sortStr;

    data["scaling"]["StretchToScreenWidth"] = stretchToScreenWidth;
    data["scaling"]["StretchToScreenHeight"] = stretchToScreenHeight;
    data["scaling"]["ShrinkToScreenWidth"] = shrinkToScreenWidth;
    data["scaling"]["ShrinkToScreenHeight"] = shrinkToScreenHeight;
}

void WindowManager::WriteConfig(toml::value& data) {
    auto root = GetExecutableDir();
    fs::path file("config.toml");
    fs::path path = root / file;

    const auto serial = toml::format(data, 0, 17);

    LOG(serial);

    write_file(path.string(), serial.c_str(), serial.size());

    return;
}
*/
void WindowManager::ReadOrigin() {
    fs::path root = GetExecutableDir();
    fs::path file("origin.data");
    fs::path path = root / file;

    std::vector<std::byte> bytes(sizeof(POINT));

    try {
        bytes = load_file(path.string());
        POINT* origin = (POINT*)&bytes[0];
        x_origin = origin->x;
        y_origin = origin->y;
    }
    catch (const std::runtime_error& e) {
        // Center of the primary screen
        int mw = GetSystemMetrics(SM_CXSCREEN);
        int mh = GetSystemMetrics(SM_CYSCREEN);
        x_origin = mw/2;
        y_origin = mh/2;
    }
}

//void WindowManager::WriteOrigin() {
//    POINT origin;
//    origin.x = x_origin;
//    origin.y = y_origin;
//
//    fs::path root = GetExecutableDir();
//    fs::path file("origin.data");
//    fs::path path = root / file;
//
//    write_file(path.string(), (const char*)&origin, sizeof(POINT));
//}

void WindowManager::UpdateOrigin() {
    UpdateOriginFromWindow(hWnd); // Update from our window's HWND
}

// Also used to get initial origin point from the toplevel window at launch
void WindowManager::UpdateOriginFromWindow(HWND hWindow) {
    if (hWindow == NULL) return;
    RECT wrc;
    GetWindowRect(hWindow, &wrc);
    int h = wrc.bottom - wrc.top;
    int w = wrc.right - wrc.left;
    x_origin = wrc.left + w / 2;
    y_origin = wrc.top + h / 2;
}

void WindowManager::ManualZoom(float mod, float absolute) {
    float old_scale = canvas.scale_effective;
    if (absolute != 0.0) {
        canvas.scale_manual = absolute;
    } else {
        canvas.scale_manual = canvas.scale_effective + mod;
    }
    if (canvas.scale_manual < 0.10f) {
        canvas.scale_manual = 0.10;
    }
    if (canvas.scale_manual > 10.0f) {
        canvas.scale_manual = 10.0;
    }
    if (canvas.scale_manual == old_scale) {
        return;
    }

    ResizeForImage();
    Redraw(RDW_ERASE);
}

void WindowManager::ManualZoomToPoint(float mod, float absolute, int zoomPointX,  int zoomPointY) {
    float old_scale = canvas.scale_effective;


    // There's a bug here. zoom point is supposed to be in window space (counting from client area)
    // But If maximized or fullscreen the whole screen is client area, but image is centered.
    // It's fixed for now by normal clamping of panning offset.

    //RECT crc;
    //GetClientRect(hWnd, &crc);
    //int ch = crc.bottom - crc.top;
    //int cw = crc.right - crc.left;

    //int cax = 0;
    //int cay = 0;
    //if (cw > canvas.w_scaled) {
    //    cax = (cw - canvas.w_scaled) / 2;
    //}

    //if (ch > canvas.h_scaled) {
    //    cay = (ch - canvas.h_scaled) / 2;
    //}
    //RECT rc;
    //GetCenteredImageRect(&rc);


    // Calculcating coordinates of the zoom point in native resolution
    float x_click_offset_native = (canvas.x_poffset + zoomPointX) / old_scale;
    float y_click_offset_native = (canvas.y_poffset + zoomPointY) / old_scale;

    // floating point zoompoint to client
    float fzpx = static_cast<float>(zoomPointX) / canvas.w_client;
    float fzpy = static_cast<float>(zoomPointY) / canvas.h_client;

    //LOG_DEBUG("--------------");
    //LOG_DEBUG("Pan: {0},{1}  ZP: {2},{3}", canvas.x_poffset, canvas.y_poffset, zoomPointX, zoomPointY);
    //LOG_DEBUG("Native point: {0},{1}", x_click_offset_native, y_click_offset_native);


    ManualZoom(mod, absolute);

    // Converting it to new scale coords
    float x_click_offset = x_click_offset_native * canvas.scale_effective;
    float y_click_offset = y_click_offset_native * canvas.scale_effective;

    // client area dimensions here are already updated to the new scale
    canvas.x_poffset = x_click_offset - ((fzpx * canvas.w_client) );
    canvas.y_poffset = y_click_offset - ((fzpy * canvas.h_client) );

    Redraw();


    LimitPanOffset();
}

void WindowManager::GetWindowSizeForImage(RECT& rrc) {

    //----------
    // 1) Find appropriate monitor for origin point
    // 1a) Don't change origin unless window was manually moved

    POINT origin;
    origin.x = x_origin;
    origin.y = y_origin;

    MONITORINFO monitor_info;
    monitor_info.cbSize = sizeof(monitor_info);
    GetMonitorInfo(MonitorFromPoint(origin, MONITOR_DEFAULTTONEAREST), &monitor_info);
    RECT& mrc = monitor_info.rcMonitor;
    RECT& mwrc = monitor_info.rcWork;

    //-----------
    // Getting monitor and window sizes

    RECT& screenrc = (isFullscreen) ? monitor_info.rcMonitor : monitor_info.rcWork;

    long style = GetWindowLong(hWnd, GWL_STYLE);


    int w_border = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
    int h_border = GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
    int h_caption = GetSystemMetrics(SM_CYCAPTION);
    int h_menu = 0;
    if (GetMenu(hWnd)) {
        h_menu = GetSystemMetrics(SM_CYMENU);
    }

    int w_screenwa = screenrc.right - screenrc.left;
    int h_screenwa = screenrc.bottom - screenrc.top;

    int hasBorder = style & (WS_CAPTION | WS_THICKFRAME);
    if (hasBorder && !isFullscreen) {
        w_screenwa -= w_border * 2;
        h_screenwa -= (h_border * 2 + h_caption + h_menu);
    }

    //printf("Metrics:\nBorder Horizontal: %i\nBorder Vertical: %i\nCaption: %i\nScreenWA W: %i\nScreen WA H: %i\n", w_border, h_border, h_caption, w_screenwa, h_screenwa);

    //-----------
    // 2a) Stretching, shrinking and scaling

    int w_native = frame->width;
    int h_native = frame->height;
    canvas.SetImageBaseRotation(frame->internalRotation);
    float rotation = canvas.GetRotation();

    if (rotation == 90.0f || rotation == 270.0f) {
        w_native = frame->height;
        h_native = frame->width;
    }

    float w_scale_candidate = 0.0;
    if (w_native < w_screenwa && stretchToScreenWidth) {
        w_scale_candidate = float(w_screenwa) / w_native;
    }

    if (w_native >= w_screenwa && shrinkToScreenWidth) {
        w_scale_candidate = float(w_screenwa) / w_native;
    }

    float h_scale_candidate = 0.0;
    if (h_native < h_screenwa && stretchToScreenHeight) {
        h_scale_candidate = float(h_screenwa) / h_native;
    }

    if (h_native >= h_screenwa && shrinkToScreenHeight) {
        h_scale_candidate = float(h_screenwa) / h_native;
    }

    float scale_candidate2;
    if (w_scale_candidate > 0.0 && h_scale_candidate > 0.0) {
        scale_candidate2 = std::min(w_scale_candidate, h_scale_candidate);
    }
    else {
        scale_candidate2 = std::max(w_scale_candidate, h_scale_candidate);
    }

    float endscale;
    if (canvas.scale_manual == 1.0 && scale_candidate2 != 0.0) {
        endscale = scale_candidate2;
    }
    else {
        endscale = canvas.scale_manual;
    }

    endscale *= canvas.scale_thumb;

    canvas.w_scaled = w_native * endscale;
    canvas.h_scaled = h_native * endscale;

    canvas.scale_effective = endscale;

    // if zoom lock is on it should override all autoscaling
    // manual scaling should start from current effective scale

    // ---------
    // 2b) Calculate new window coords around that origin

    // Separate borderless border stuff
    //int bb = (!hasBorder && !isFullscreen) ? borderlessBorder : 0;

    int cut_width = std::min(canvas.w_scaled, w_screenwa);
    int cut_height = std::min(canvas.h_scaled, h_screenwa);

    RECT new_client_area;

    new_client_area.left = origin.x - cut_width / 2;
    new_client_area.right = (origin.x + cut_width) - cut_width / 2;
    new_client_area.top = origin.y - cut_height / 2;
    new_client_area.bottom = (origin.y + cut_height) - cut_height / 2;

    canvas.h_client = new_client_area.bottom - new_client_area.top;
    canvas.w_client = new_client_area.right - new_client_area.left;

    LimitPanOffset();

    AdjustWindowRect(&new_client_area, style, false);
    //if (hasBorder) {
    //    //Doing AdjustWindowRect manually, because it wasn't working out for some reason
    //    new_client_area.left -= w_border;
    //    new_client_area.right += w_border;
    //    new_client_area.top -= (h_border + h_caption + h_menu);
    //    new_client_area.bottom += h_border;
    //}

    //-------------
    // 3) Clamp new rect to that monitor rect

    RECT& new_window_area = new_client_area;
    if (new_window_area.right > screenrc.right) {
        int diff = new_window_area.right - screenrc.right;
        new_window_area.right -= diff;
        new_window_area.left-= diff;
    }

    if (new_window_area.left < screenrc.left) {
        int diff = new_window_area.left - screenrc.left;
        new_window_area.right -= diff;
        new_window_area.left -= diff;
    }

    if (new_window_area.bottom > screenrc.bottom) {
        int diff = new_window_area.bottom - screenrc.bottom;
        new_window_area.bottom -= diff;
        new_window_area.top -= diff;
    }

    if (new_window_area.top < screenrc.top) {
        int diff = new_window_area.top - screenrc.top;
        new_window_area.bottom -= diff;
        new_window_area.top -= diff;
    }

    rrc.left = new_window_area.left;
    rrc.top = new_window_area.top;
    rrc.right = new_window_area.right;
    rrc.bottom = new_window_area.bottom;


}

void WindowManager::ResizeForImage() {
    RECT new_window_area;
    GetWindowSizeForImage(new_window_area);

    int x = new_window_area.left;
    int y = new_window_area.top;
    int w = new_window_area.right - new_window_area.left;
    int h = new_window_area.bottom - new_window_area.top;

    //--------------

    _TouchSizeEventTimestamp();

    bool dynamicPos = !isMaximized && !isFullscreen;
    if (dynamicPos) {

        SetWindowPos(hWnd, HWND_TOP, x, y, w, h, SWP_DEFERERASE | SWP_NOCOPYBITS); // These flags are pretty good to reduce flickering and discarding old bits
        LOG("<RESIZE> C: {0}x{1}    W: {2}x{3}", canvas.w_client, canvas.h_client, w, h);
    }

    //UpdateWindowSizeInfo();

}

void WindowManager::ShowPopupMenu(POINT& p) {
    HMENU popupRootMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDC_D4SEE_POPUP));
    HMENU popupMenu = GetSubMenu(popupRootMenu, 0);
    RemoveMenu(popupRootMenu, 0, MF_BYPOSITION);
    DestroyMenu(popupRootMenu);


    CheckMenuItem(popupMenu, ID_ALWAYSONTOP, MF_BYCOMMAND | BOOLCOMMANDCHECK(isAlwaysOnTop));
    CheckMenuItem(popupMenu, ID_ZOOMLOCK, MF_BYCOMMAND | BOOLCOMMANDCHECK(zoomLock));
    CheckMenuItem(popupMenu, ID_SHRINKTOWIDTH, MF_BYCOMMAND | BOOLCOMMANDCHECK(shrinkToScreenWidth));
    CheckMenuItem(popupMenu, ID_SHRINKTOHEIGHT, MF_BYCOMMAND | BOOLCOMMANDCHECK(shrinkToScreenHeight));
    CheckMenuItem(popupMenu, ID_STRETCHTOWIDTH, MF_BYCOMMAND | BOOLCOMMANDCHECK(stretchToScreenWidth));
    CheckMenuItem(popupMenu, ID_STRETCHTOHEIGHT, MF_BYCOMMAND | BOOLCOMMANDCHECK(stretchToScreenHeight));
    CheckMenuItem(popupMenu, ID_SORTBY_NAME, MF_BYCOMMAND | BOOLCOMMANDCHECK(sortMethod == PlaylistSortMethod::ByName));
    CheckMenuItem(popupMenu, ID_SORTBY_DATEMODIFIED, MF_BYCOMMAND | BOOLCOMMANDCHECK(sortMethod == PlaylistSortMethod::ByDateModified));
    EnableMenuItem(popupMenu, ID_ACTUALSIZE, MF_BYCOMMAND | BOOLCOMMANDENABLE(canvas.scale_manual != 1.0f));



    TrackPopupMenu(popupMenu, TPM_LEFTBUTTON, p.x, p.y, 0, hWnd, 0);
    DestroyMenu(popupMenu);
}

void WindowManager::HandleMenuCommand(unsigned int uIDItem) {
    switch (uIDItem) {
        case ID_OPENFILE: {
            std::wstring filepath = OpenFileDialog();
            if (filepath != L"") {
                playlist.GeneratePlaylist(filepath);
                SelectImage(std::make_shared<ImageContainer>(hWnd, playlist.Current()->path, playlist.Current()->format));
                //std::cout << filepath.c_str() << std::endl;
            }
            break;
        }
        case ID_ZOOMLOCK: {
            zoomLock = !zoomLock;
            break;
        }
        case ID_ACTUALSIZE: {
            ManualZoom(+0.0f, 1.0f);
            break;
        }
        case ID_ZOOMIN: {
            ManualZoom(+0.10f);
            break;
        }
        case ID_ZOOMOUT: {
            ManualZoom(-0.10f);
            break;
        }
        case ID_ROTATECW: {
            canvas.CycleRotation(+1);

            ResizeForImage();
            Redraw(RDW_ERASE);
            break;
        }
        case ID_SHRINKTOWIDTH: {
            shrinkToScreenWidth = !shrinkToScreenWidth;
            ResizeForImage();
            Redraw(RDW_ERASE);
            break;
        }
        case ID_SHRINKTOHEIGHT: {
            shrinkToScreenHeight = !shrinkToScreenHeight;
            ResizeForImage();
            Redraw(RDW_ERASE);
            break;
        }
        case ID_STRETCHTOWIDTH: {
            stretchToScreenWidth = !stretchToScreenWidth;
            ResizeForImage();
            Redraw(RDW_ERASE);
            break;
        }
        case ID_STRETCHTOHEIGHT: {
            stretchToScreenHeight = !stretchToScreenHeight;
            ResizeForImage();
            Redraw(RDW_ERASE);
            break;
        }
        case ID_ALWAYSONTOP: {
            ToggleAlwaysOnTop();
            break;
        }
        case ID_TOGGLEFULLSCREEN: {
            ToggleFullscreen();
            break;
        }
        case ID_TOGGLEBORDERLESS: {
            ToggleBorderless();
            break;
        }
        case ID_SORTBY_NAME: {
            sortMethod = PlaylistSortMethod::ByName;
            playlist.SetSortingMethod(sortMethod);
            break;
        }
        case ID_SORTBY_DATEMODIFIED: {
            sortMethod = PlaylistSortMethod::ByDateModified;
            playlist.SetSortingMethod(sortMethod);
            break;
        }
        case ID_ROOT_SETTINGS:
            DialogBox(NULL, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT: {
            DestroyWindow(hWnd);
            break;
        }
    }
}