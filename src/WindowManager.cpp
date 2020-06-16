#include "WindowManager.h"
#include <fstream>

namespace fs = std::filesystem;

void WindowManager::Pan(int x, int y) {
    x_poffset += x;
    int x_limit = w_scaled - w_client;
    x_poffset = std::min(x_poffset, x_limit);
    x_poffset = std::max(x_poffset, 0);

    y_poffset += y;
    int y_limit = h_scaled - h_client;
    y_poffset = std::min(y_poffset, y_limit);
    y_poffset = std::max(y_poffset, 0);

    fastDrawDone = false;
    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
}

void WindowManager::GetWindowSize() {
    RECT warc;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &warc, 0);
    h_working_area = warc.bottom - warc.top;
    w_working_area = warc.right - warc.left;

    RECT crc;
    GetClientRect(hWnd, &crc);
    h_client = crc.bottom - crc.top;
    w_client = crc.right - crc.left;
    // Used for panning

    RECT wrc;
    GetWindowRect(hWnd, &wrc);
    h_window = wrc.bottom - wrc.top;
    w_window = wrc.right - wrc.left;

    w_border = GetSystemMetrics(SM_CXBORDER);
    h_border = GetSystemMetrics(SM_CYBORDER);
    h_caption = GetSystemMetrics(SM_CYCAPTION);

    //x_origin = crc.left + w_client / 2;
    //y_origin = crc.top + h_client / 2;

    MONITORINFO monitor_info;
    monitor_info.cbSize = sizeof(monitor_info);
    GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &monitor_info);
    RECT& mrc = monitor_info.rcMonitor;



    //w_border = GetSystemMetrics(SM_CXSIZEFRAME);;
    //h_border = GetSystemMetrics(SM_CYSIZEFRAME);;
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

bool WindowManager::SaveWindowParams() {
    long style = GetWindowLong(hWnd, GWL_STYLE);
    stash.isMaximized = style & WS_MAXIMIZE;

    if (stash.isMaximized) {
        //SendMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
        return stash.isMaximized;
    }

    stash.style = GetWindowLong(hWnd, GWL_STYLE);
    stash.exstyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    GetWindowRect(hWnd, &stash.rc);
    return stash.isMaximized;
}


void WindowManager::ToggleFullscreen() {
    if (!isFullscreen) {
        bool isCurrentlyFullscreen = SaveWindowParams();
        
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
        RECT* rc = &stash.rc;
        int w = rc->right - rc->left;
        int h = rc->bottom - rc->top;
        SetWindowPos(hWnd, NULL, rc->left, rc->top, w, h, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

        if (stash.isMaximized)
            ::SendMessage(hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
    }
}

void WindowManager::SelectFrame(MemoryFrame* f) {
    frame = f;
    x_poffset = 0;
    y_poffset = 0;
}

std::filesystem::path GetExecutableDir() {
    std::filesystem::path exePath;

    WCHAR path[1000];
    GetModuleFileNameW(GetModuleHandle(NULL), path, 1000);

    exePath = path;
    return exePath.parent_path();
}

std::filesystem::path GetAppDataRoaming() {
    std::filesystem::path configRoot;
    PWSTR path_tmp;

    if (S_OK != SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path_tmp)) {
        CoTaskMemFree(path_tmp); // Free memory
        return GetExecutableDir();
    }
    configRoot = path_tmp;
    CoTaskMemFree(path_tmp); // Free memory

    return configRoot;

    //std::filesystem::path appFolder("D4See");
    //std::filesystem::path file("origin");

    //std::filesystem::path fullPath = configRoot / appFolder / file;
    //std::cout << fullPath << std::endl;
}

std::vector<std::byte> load_file(std::string const& filepath)
{
    std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);

    if (!ifs)
        throw std::runtime_error(filepath + ": " + std::strerror(errno));

    auto end = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    auto size = std::size_t(end - ifs.tellg());

    if (size == 0) // avoid undefined behavior 
        return {};

    std::vector<std::byte> buffer(size);

    if (!ifs.read((char*)buffer.data(), buffer.size()))
        throw std::runtime_error(filepath + ": " + std::strerror(errno));

    //ifs.close();

    return buffer;
}

int write_file(std::string const& filepath, const char* data, unsigned int size)
{
    std::ofstream ofs(filepath, std::ios::out | std::ios::binary);

    if (!ofs)
        throw std::runtime_error(filepath + ": " + std::strerror(errno));

    ofs.write(data, size);

    std::vector<std::byte> buffer(size);

    //ofs.close();

    return 1;
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

void WindowManager::ReadOrigin() {
    fs::path root = GetExecutableDir();
    fs::path file("origin.data");
    fs::path path = root / file;

    std::vector<std::byte> bytes(sizeof(POINT));
    bytes = load_file(path.string());

    POINT* origin = (POINT*)&bytes[0];
    x_origin = origin->x;
    y_origin = origin->y;
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

void WindowManager::ResizeForImage() {

    w_scaled = frame->image->xres * scale;
    h_scaled = frame->image->yres * scale;

    int scw = frame->image->xres;
    int sch = frame->image->yres;


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
    // 2) Calculate new window coords around that origin

    long style = GetWindowLong(hWnd, GWL_STYLE);

    int monitor_wawidth = mwrc.right - mwrc.left;
    int monitor_waheight = mwrc.bottom - mwrc.top;

    int hasBorder = style & WS_OVERLAPPEDWINDOW;
    if (hasBorder) {
        monitor_wawidth -= w_border * 2;
        monitor_waheight -= (h_border * 2 + h_caption);
    }

    int cut_width = std::min(w_scaled, monitor_wawidth);
    int cut_height = std::min(h_scaled, monitor_waheight);

    RECT new_client_area;
    
    new_client_area.left = origin.x - cut_width / 2;
    new_client_area.right = (origin.x + cut_width) - cut_width / 2;
    new_client_area.top = origin.y - cut_height / 2;
    new_client_area.bottom = (origin.y + cut_height) - cut_height / 2;

    //AdjustWindowRect(&new_client_area, style, false);
    if (hasBorder) {
        // Doing AdjustWindowRect manually, because it wasn't working out for some reason
        new_client_area.left -= w_border;
        new_client_area.right += w_border;
        new_client_area.top -= h_border + h_caption;
        new_client_area.bottom += h_border;
    }

    //-------------
    // 3) Clamp new rect to that monitor rect

    RECT& new_window_area = new_client_area;
    if (new_window_area.right > mwrc.right) {
        int diff = new_window_area.right - mwrc.right;
        new_window_area.right -= diff;
        new_window_area.left-= diff;
    }    

    if (new_window_area.left < mwrc.left) {
        int diff = new_window_area.left - mwrc.left;
        new_window_area.right -= diff;
        new_window_area.left -= diff;
    }

    if (new_window_area.bottom > mwrc.bottom) {
        int diff = new_window_area.bottom - mwrc.bottom;
        new_window_area.bottom -= diff;
        new_window_area.top -= diff;
    }

    if (new_window_area.top < mwrc.top) {
        int diff = new_window_area.top - mwrc.top;
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


        //int w_wa2 = w_working_area - w_border * 2; // max client area size. WA minus window borders
        //int h_wa2 = h_working_area - (h_border * 2 + h_caption);

        //if (w_scaled < w_wa2) {
        //    x = (w_wa2 - w_scaled) / 2;
        //    w = w_scaled + w_border * 2;
        //}
        //else w = w_working_area;

        //if (h_scaled < h_wa2) {
        //    y = (h_wa2 - h_scaled) / 2;
        //    h = h_scaled + (h_border * 2 + h_caption);
        //}
        //else h = h_working_area;

    if (dynamicPos) {

        SetWindowPos(hWnd, HWND_TOP, x, y, w, h, SWP_DEFERERASE | SWP_NOCOPYBITS); // These flags are pretty good to reduce flickering and discarding old bits
        std::cout << "RESIZE" << std::endl;
    }
    else if (isFullscreen) {
        stash.rc.left = x;
        stash.rc.top = y;
        stash.rc.right = x + w;
        stash.rc.bottom = y + h;
    }

    fastDrawDone = false; // Fill paint LQ version fast on the next redraw
    frame->drawId--; // Will cause the redraw again after the first, and it'll be HQ
    //RECT rc;
    //rc.top = 0;
    //rc.left = 0;
    //rc.right = scw;
    //rc.bottom = sch;
    //AdjustWindowRect(&rc, WS_CAPTION, false);
    //MoveWindow(hWnd, rc.left, rc.top, rc.right, rc.bottom, false);

    GetWindowSize();
}