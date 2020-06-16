#include "WindowManager.h"

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

    RECT rc;
    GetClientRect(hWnd, &rc);
    h_client = rc.bottom - rc.top;
    w_client = rc.right - rc.left;
    // Used for panning

    RECT wrc;
    GetWindowRect(hWnd, &wrc);
    h_window = wrc.bottom - wrc.top;
    w_window = wrc.right - wrc.left;

    w_border = GetSystemMetrics(SM_CXBORDER);
    h_border = GetSystemMetrics(SM_CYBORDER);
    h_caption = GetSystemMetrics(SM_CYCAPTION);

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

void WindowManager::ResizeForImage() {
    int x = 0;
    int y = 0;
    int w;
    int h;

    w_scaled = frame->image->xres * scale;
    h_scaled = frame->image->yres * scale;

    int scw = frame->image->xres;
    int sch = frame->image->yres;

    // 1) Find appropriate monitor from previous window rect
    // 2) Find it's central origin point
    // 3) Make new frame around that origin on an appopriate monitor
    // 3a) Don't change origin unless window was manually moved
    // 4) Clamp new rect to that monitor rect

    bool dynamicPos = !isMaximized && !isFullscreen;


        int w_wa2 = w_working_area - w_border * 2; // max client area size. WA minus window borders
        int h_wa2 = h_working_area - (h_border * 2 + h_caption);

        if (w_scaled < w_wa2) {
            x = (w_wa2 - w_scaled) / 2;
            w = w_scaled + w_border * 2;
        }
        else w = w_working_area;

        if (h_scaled < h_wa2) {
            y = (h_wa2 - h_scaled) / 2;
            h = h_scaled + (h_border * 2 + h_caption);
        }
        else h = h_working_area;

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