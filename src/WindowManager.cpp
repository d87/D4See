#include "WindowManager.h"

namespace fs = std::filesystem;

void WindowManager::Redraw(unsigned int addFlags) {
    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | addFlags);
}

void WindowManager::ScheduleRedraw(unsigned int ms) {
    SetTimer(hWnd, D4S_TIMER_HQREDRAW, ms, NULL);
}
void WindowManager::StopTimer(UINT_PTR id) {
    KillTimer(hWnd, id);
}

WindowManager::~WindowManager() {
    if (playlist) delete playlist;
    if (frame) delete frame;
    if (frame2) delete frame2;
}

//void ClearWindowForFrame(HWND hWnd, MemoryFrame* f) {
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
    SelectFrame(frame2);
    frame2 = nullptr;
    //frame->drawId = frame->decoderBatchId;
    //gWinMgr.newImagePending = true;

    //ClearWindowForFrame(hWnd, frame);

    using namespace std::chrono_literals;
    auto status = frame->threadInitFinished.wait_for(2ms);
    if (status == std::future_status::ready) {
        ResizeForImage();
        Redraw(RDW_ERASE);
    }
}

void WindowManager::DiscardPrefetch() {
    if (frame2)
        delete frame2;
    frame2 = nullptr;
}

void WindowManager::StartPrefetch(MemoryFrame* f) {
    frame2 = f;
    // Start 2 minute timer
    SetTimer(hWnd, D4S_PREFETCH_TIMEOUT, 120000, NULL);
}

void WindowManager::PreviousImage() {
    if (playlist->Move(PlaylistPos::Current, -1)) {
        LoadImage(-1);
    }
}

void WindowManager::NextImage() {
    if (playlist->Move(PlaylistPos::Current, +1)) {
        LoadImage(+1);
    }
}

void WindowManager::LoadImage(int prefetchDir) {
    PlaylistEntry* cur = playlist->Current();
    PlaylistEntry* following = nullptr;
    if (prefetchDir > 0)
        following = playlist->Next();
    else if (prefetchDir < 0)
        following = playlist->Prev();

    if (cur) {
        bool prefetchHit = false;
        if (frame2)
            if (frame2->filename == wide_to_utf8(cur->path))
                prefetchHit = true;
        if (prefetchHit) {
            ShowPrefetch();
        }
        else { // Changed direction or jumped more than 1
            DiscardPrefetch();
            SelectFrame(new MemoryFrame(hWnd, cur->path, cur->format));
        }
    }
    if (following) {
        StartPrefetch(new MemoryFrame(hWnd, following->path, following->format));
    }
    else {
        frame2 = nullptr;
    }
}

void WindowManager::LimitPanOffset() {
    int x_limit = w_scaled - w_client;
    x_poffset = std::min(x_poffset, x_limit);
    x_poffset = std::max(x_poffset, 0);
    
    int y_limit = h_scaled - h_client;
    y_poffset = std::min(y_poffset, y_limit);
    y_poffset = std::max(y_poffset, 0);
}


void WindowManager::Pan(int x, int y) {
    x_poffset += x;
    y_poffset += y;
    LimitPanOffset();

    fastDrawDone = false;
    this->Redraw(); // 
    this->ScheduleRedraw(50); // HQ redraw later
    //RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
}

void WindowManager::UpdateWindowSizeInfo() {
    // Used only to update client area dimensions after WM_SIZE to limit panning offset
    RECT crc;
    GetClientRect(hWnd, &crc);
    h_client = crc.bottom - crc.top;
    w_client = crc.right - crc.left;
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
    if (cw > w_scaled) {
        x = (cw - w_scaled) / 2;
    }

    if (ch > h_scaled) {
        y = (ch - h_scaled) / 2;
    }
    rc->left = x;
    rc->top = y;
    rc->right = x + w_scaled;
    rc->bottom = y + h_scaled;
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
        ResizeForImage();

        if (stash.isMaximized)
            ::SendMessage(hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
    }
}

void WindowManager::ToggleBorderless(int doRedraw) {
    if (isFullscreen) return;

    if (!isBorderless) {
        SaveWindowParams(&this->borderlessStash);
    }

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
        ScheduleRedraw(50);
    }
}

void WindowManager::SelectFrame(MemoryFrame* f) {
    if (frame != nullptr) {
        delete frame;
    }

    std::cout << "<<<<<<< FRAME SWITCH >>>>>>>" << std::endl;
    frame = f;
    fastDrawDone = false;
    //ScheduleRedraw(50);
    StopTimer(D4S_TIMER_HQREDRAW);
    x_poffset = 0;
    y_poffset = 0;
    if (!zoomLock) {
        scale_manual = 1.0f;
    }
}

void WindowManager::SelectPlaylist(Playlist* playlist) {
    if (this->playlist != nullptr) {
        delete this->playlist;
    }
    this->playlist = playlist;
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


toml::value WindowManager::ReadConfig() {

    // TODO: Error handling of all sorts

    auto root = GetExecutableDir();
    fs::path file("config.toml");
    fs::path path = root / file;

    toml::value data;
    try {
        data = toml::parse(path.string());
        //auto enabled = data["scaling"]["StretchToScreenWidth"].as_boolean();
        std::cout << data << std::endl;
    }
    catch (const std::runtime_error& e) {
        std::cout << "No config file" << std::endl;
        const toml::value data{
            {"scaling", {{"StretchToScreenWidth", true}, {"StretchToScreenHeight", true}, {"ShrinkToScreenWidth", true}, {"ShrinkToScreenHeight", true}, }}
        };
    }
    return data;
}

void WindowManager::RestoreConfigValues(toml::value& data) {
    if (data["general"]["StartBorderless"].as_boolean()) {
        ToggleBorderless(0);
    }

    std::string sortStr = data["general"]["SortMethod"].as_string();
    if (sortStr == "ByName") {
        sortMethod = PlaylistSortMethod::ByName;
    }
    else if (sortStr == "ByDateModified") {
        sortMethod = PlaylistSortMethod::ByDateModified;
    }

    stretchToScreenWidth = data["scaling"]["StretchToScreenWidth"].as_boolean();
    stretchToScreenHeight = data["scaling"]["StretchToScreenHeight"].as_boolean();
    shrinkToScreenWidth = data["scaling"]["ShrinkToScreenWidth"].as_boolean();
    shrinkToScreenHeight = data["scaling"]["ShrinkToScreenHeight"].as_boolean();
}

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

    const auto serial = toml::format(data, /*width = */ 0, /*prec = */ 17);

    std::cout << serial << std::endl;

    write_file(path.string(), serial.c_str(), serial.size());

    return;
}

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
        int mw = GetSystemMetrics(SM_CXSCREEN);
        int mh = GetSystemMetrics(SM_CYSCREEN);
        x_origin = mw/2;
        y_origin = mh/2;
    }    
}

void WindowManager::WriteOrigin() {
    POINT origin;
    origin.x = x_origin;
    origin.y = y_origin;

    fs::path root = GetExecutableDir();
    fs::path file("origin.data");
    fs::path path = root / file;

    std::cout << "Writing origin " << x_origin << "," << y_origin << std::endl;
    
    write_file(path.string(), (const char*)&origin, sizeof(POINT));
}

void WindowManager::UpdateOrigin() {
    RECT wrc;
    GetWindowRect(hWnd, &wrc);
    int h = wrc.bottom - wrc.top;
    int w = wrc.right - wrc.left;
    x_origin = wrc.left + w / 2;
    y_origin = wrc.top + h / 2;
}

void WindowManager::ManualZoom(float mod, float absolute) {
    float old_scale = scale_effective;
    if (absolute != 0.0) {
        scale_manual = absolute;
    } else {
        scale_manual = scale_effective + mod;
        if (scale_manual < 0.10f) {
            scale_manual = 0.10;
        }
        if (scale_manual > 10.0f) {
            scale_manual = 10.0;
        }
    }
    if (scale_manual == old_scale) {
        return;
    }

    ResizeForImage();
    Redraw(RDW_ERASE);
    ScheduleRedraw(50);
}

void WindowManager::ResizeForImage( bool HQRedraw) {

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

    int w_native = frame->image->xres;
    int h_native = frame->image->yres;

    float w_scale_candidate = 0.0;
    if (w_native < w_screenwa && stretchToScreenWidth) {
        w_scale_candidate = float(w_screenwa) / w_native;
        std::cout << "Upscale horizonal candidate " << w_scale_candidate << std::endl;
    }

    if (w_native >= w_screenwa && shrinkToScreenWidth) {
        w_scale_candidate = float(w_screenwa) / w_native;
        std::cout << "Downscale horizonal candidate " << w_scale_candidate << std::endl;
    }

    float h_scale_candidate = 0.0;
    if (h_native < h_screenwa && stretchToScreenHeight) {
        h_scale_candidate = float(h_screenwa) / h_native;
        std::cout << "Upscale horizonal candidate " << h_scale_candidate << std::endl;
    }

    if (h_native >= h_screenwa && shrinkToScreenHeight) {
        h_scale_candidate = float(h_screenwa) / h_native;
        std::cout << "Downscale horizonal candidate " << h_scale_candidate << std::endl;
    }

    float scale_candidate2;
    if (w_scale_candidate > 0.0 && h_scale_candidate > 0.0) {
        scale_candidate2 = std::min(w_scale_candidate, h_scale_candidate);
    }
    else {
        scale_candidate2 = std::max(w_scale_candidate, h_scale_candidate);
    }
    std::cout << "Comparing candidates " << w_scale_candidate << " " << h_scale_candidate << "=" << scale_candidate2 << std::endl;

    float endscale;
    if (scale_manual == 1.0 && scale_candidate2 != 0.0) {
        endscale = scale_candidate2;
    }
    else {
        endscale = scale_manual;
    }

    w_scaled = w_native * endscale;
    h_scaled = h_native * endscale;

    scale_effective = endscale;

    // if zoom lock is on it should override all autoscaling
    // manual scaling should start from current effective scale
    
    // ---------
    // 2b) Calculate new window coords around that origin

    int cut_width = std::min(w_scaled, w_screenwa);
    int cut_height = std::min(h_scaled, h_screenwa);

    RECT new_client_area;
    
    new_client_area.left = origin.x - cut_width / 2;
    new_client_area.right = (origin.x + cut_width) - cut_width / 2;
    new_client_area.top = origin.y - cut_height / 2;
    new_client_area.bottom = (origin.y + cut_height) - cut_height / 2;

    h_client = new_client_area.bottom - new_client_area.top;
    w_client = new_client_area.right - new_client_area.left;

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

    int x = new_window_area.left;
    int y = new_window_area.top;
    int w = new_window_area.right - new_window_area.left;
    int h = new_window_area.bottom - new_window_area.top;


    //--------------
    
    _TouchSizeEventTimestamp();

    bool dynamicPos = !isMaximized && !isFullscreen;


    if (dynamicPos) {

        SetWindowPos(hWnd, HWND_TOP, x, y, w, h, SWP_DEFERERASE | SWP_NOCOPYBITS); // These flags are pretty good to reduce flickering and discarding old bits
        //std::cout << "RESIZE" << " C: " << w_client << "x" << h_client << " W: " << w << "x" << h<< " N: " << w_native << "x" << h_native << std::endl;
    }

    fastDrawDone = HQRedraw; // Normally false, Fill paint LQ version fast on the next redraw
    //frame->drawId--; // Will cause the redraw again after the first, and it'll be HQ
    //RECT rc;
    //rc.top = 0;
    //rc.left = 0;
    //rc.right = scw;
    //rc.bottom = sch;
    //AdjustWindowRect(&rc, WS_CAPTION, false);
    //MoveWindow(hWnd, rc.left, rc.top, rc.right, rc.bottom, false);
    //UpdateWindowSizeInfo();
}

void WindowManager::ShowPopupMenu(POINT& p) {
    HMENU popupRootMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDC_D4SEE_POPUP));
    HMENU popupMenu = GetSubMenu(popupRootMenu, 0);
    RemoveMenu(popupRootMenu, 0, MF_BYPOSITION);
    DestroyMenu(popupRootMenu);


    CheckMenuItem(popupMenu, ID_ALWAYSONTOP, MF_BYCOMMAND | BOOLCOMMANDCHECK(alwaysOnTop));
    CheckMenuItem(popupMenu, ID_ZOOMLOCK, MF_BYCOMMAND | BOOLCOMMANDCHECK(zoomLock));
    CheckMenuItem(popupMenu, ID_SHRINKTOWIDTH, MF_BYCOMMAND | BOOLCOMMANDCHECK(shrinkToScreenWidth));
    CheckMenuItem(popupMenu, ID_SHRINKTOHEIGHT, MF_BYCOMMAND | BOOLCOMMANDCHECK(shrinkToScreenHeight));
    CheckMenuItem(popupMenu, ID_STRETCHTOWIDTH, MF_BYCOMMAND | BOOLCOMMANDCHECK(stretchToScreenWidth));
    CheckMenuItem(popupMenu, ID_STRETCHTOHEIGHT, MF_BYCOMMAND | BOOLCOMMANDCHECK(stretchToScreenHeight));
    CheckMenuItem(popupMenu, ID_SORTBY_NAME, MF_BYCOMMAND | BOOLCOMMANDCHECK(sortMethod == PlaylistSortMethod::ByName));
    CheckMenuItem(popupMenu, ID_SORTBY_DATEMODIFIED, MF_BYCOMMAND | BOOLCOMMANDCHECK(sortMethod == PlaylistSortMethod::ByDateModified));
    EnableMenuItem(popupMenu, ID_ACTUALSIZE, MF_BYCOMMAND | BOOLCOMMANDENABLE(scale_manual != 1.0f));
    


    TrackPopupMenu(popupMenu, TPM_LEFTBUTTON, p.x, p.y, 0, hWnd, 0);
    DestroyMenu(popupMenu);
}

std::wstring OpenFileDialog() {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
        COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileOpenDialog* pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
            IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

        if (SUCCEEDED(hr))
        {
            // Show the Open dialog box.
            hr = pFileOpen->Show(NULL);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr))
            {
                IShellItem* pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // Display the file name to the user.
                    if (SUCCEEDED(hr))
                    {
                        std::wstring filepath(pszFilePath);
                        //MessageBoxW(NULL, pszFilePath, L"File Path", MB_OK);
                        CoTaskMemFree(pszFilePath);
                        return filepath;
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }
    return L"";
}

void WindowManager::HandleMenuCommand(unsigned int uIDItem) {
    switch (uIDItem) {
        case ID_OPENFILE: {
            std::wstring filepath = OpenFileDialog();
            if (filepath != L"") {
                auto pl = new Playlist(filepath);
                SelectPlaylist(pl);
                SelectFrame(new MemoryFrame(hWnd, pl->Current()->path, pl->Current()->format));
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
        case ID_SHRINKTOWIDTH: {
            shrinkToScreenWidth = !shrinkToScreenWidth;
            ResizeForImage(true);
            Redraw(RDW_ERASE);
            break;
        }
        case ID_SHRINKTOHEIGHT: {
            shrinkToScreenHeight = !shrinkToScreenHeight;
            ResizeForImage(true);
            Redraw(RDW_ERASE);
            break;
        }
        case ID_STRETCHTOWIDTH: {
            stretchToScreenWidth = !stretchToScreenWidth;
            ResizeForImage(true);
            Redraw(RDW_ERASE);
            break;
        }
        case ID_STRETCHTOHEIGHT: {
            stretchToScreenHeight = !stretchToScreenHeight;
            ResizeForImage(true);
            Redraw(RDW_ERASE);
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
            playlist->ChangeSortingMethod(sortMethod);
            break;
        }
        case ID_SORTBY_DATEMODIFIED: {
            sortMethod = PlaylistSortMethod::ByDateModified;
            playlist->ChangeSortingMethod(sortMethod);
            break;
        }
        //case IDM_ABOUT:
        //    DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
        //    break;
        case IDM_EXIT: {
            DestroyWindow(hWnd);
            break;
        }
    }
}