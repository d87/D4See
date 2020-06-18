//#include <stdafx.h>
#define WIN32_LEAN_AND_MEAN 

#include <windows.h>
#include <windowsx.h>
#include <objidl.h>
#include <shellapi.h>

#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")


#undef min // oiio got macro conflicts with gdi
#undef max
#include <OpenImageIO/imageio.h>
#pragma comment (lib,"OpenImageIO.lib")
#pragma comment (lib,"OpenImageIO_Util.lib")

#include <chrono>

#include "util.h"
#include "playlist.h"
#include "D4See.h"
#include "ImageBuffer.h"
#include "MemoryFrame.h"
#include "WindowManager.h"


WindowManager gWinMgr;

MemoryFrame* frame = nullptr;
MemoryFrame* frame2 = nullptr;

Playlist* playlist;


//void ClearWindow(HDC hdc) {
//    HBRUSH newBrush = CreateSolidBrush(RGB(100, 100, 100));
//    HGDIOBJ oldBrush = SelectObject(hdc, newBrush);
//
//    Rectangle(hdc, 0, 0, frame->image->xres, frame->image->yres);
//
//    SelectObject(hdc, oldBrush);
//    DeleteObject(newBrush);
//}

void ClearWindowForFrame(HWND hWnd, MemoryFrame *f) {

    //HBRUSH newBrush = CreateSolidBrush(RGB(80, 80, 80));
    //FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
    //FillRect(hdc, &rc, newBrush);
    //DeleteObject(newBrush);
    gWinMgr.ResizeForImage();


    //GdiFlush();

    // It's a bit better with ERASENOW, but it barely matters
    //RedrawWindow(hWnd, NULL, NULL, RDW_UPDATENOW|RDW_INVALIDATE);
    RedrawWindow(hWnd, NULL, NULL, RDW_ERASE| RDW_INVALIDATE);
    //RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
}

void PreviousImage(HWND hWnd) {
    if (playlist->Move(-1)) {
        PlaylistEntry* cur = playlist->Current();
        PlaylistEntry* following = playlist->Prev();
        if (cur) {
            delete frame;
            bool prefetchHit = false;
            if (frame2)
                if (frame2->filename == wide_to_utf8(cur->filename))
                    prefetchHit = true;
            if (prefetchHit) {
                frame = frame2;
                gWinMgr.SelectFrame(frame);
                //frame->drawId = frame->decoderBatchId;
                //gWinMgr.newImagePending = true;

                //ClearWindowForFrame(hWnd, frame);

                using namespace std::chrono_literals;
                auto status = frame->threadInitFinished.wait_for(2ms);
                if (status == std::future_status::ready) {
                    ClearWindowForFrame(hWnd, frame);
                }
            }
            else { // Changed direction or jumped more than 1
                frame = new MemoryFrame(hWnd, cur->filename, cur->format);
                gWinMgr.SelectFrame(frame);
                delete frame2;
            }
            frame2 = nullptr;
        }
        if (following) {
            frame2 = new MemoryFrame(hWnd, following->filename, following->format);
        }
        else {
            frame2 = nullptr;
        }
    }
}

void NextImage(HWND hWnd) {
    if (playlist->Move(1)) {
        PlaylistEntry* cur = playlist->Current();
        PlaylistEntry* following = playlist->Next();
        if (cur) {
            delete frame;
            bool prefetchHit = false;
            if (frame2)
                if (frame2->filename == wide_to_utf8(cur->filename))
                    prefetchHit = true;
            if (prefetchHit) {
                frame = frame2;
                gWinMgr.SelectFrame(frame);
                //frame->drawId = frame->decoderBatchId;
                //gWinMgr.newImagePending = true;

                //ClearWindowForFrame(hWnd, frame);

                using namespace std::chrono_literals;
                auto status = frame->threadInitFinished.wait_for(2ms);
                if (status == std::future_status::ready) {
                    ClearWindowForFrame(hWnd, frame);
                }
            }
            else { // Changed direction or jumped more than 1
                frame = new MemoryFrame(hWnd, cur->filename, cur->format);
                gWinMgr.SelectFrame(frame);
                delete frame2;
            }
            frame2 = nullptr;
        }
        if (following) {
            frame2 = new MemoryFrame(hWnd, following->filename, following->format);
        }
        else {
            frame2 = nullptr;
        }
    }
}

VOID OnPaint(HDC hdc)
{
    using namespace std::chrono_literals;

    if (frame) {
        //if (gWinMgr.newImagePending) {
        //    gWinMgr.newImagePending = false;
        //    if (frame->threadState == 0) {
        //        std::cout << "Clearing before bitmap is ready" << std::endl;
        //    }
        //    ClearWindow(hdc);
        //    gWinMgr.ResizeForImage(frame);
        //}

        if (frame->threadState > 0) {
            
            DIBImage* pImage = frame->GetActiveSubimage();

            int width = pImage->width;
            int height = pImage->height;

            bool imageComplete = frame->image->IsSubimageLoaded(frame->curFrame);
          
            if (!gWinMgr.fastDrawDone || gWinMgr.isMovingOrSizing || !imageComplete || frame->isAnimated) {
            //if (true) {
                
            
                HGDIOBJ oldbmp = SelectObject(pImage->hdc, pImage->hBitmap);
                //BitBlt(hdc, 0, 0, width, height, pImage->hdc, 0, 0, SRCCOPY);
                SetStretchBltMode(hdc, COLORONCOLOR);
                if (gWinMgr.isMaximized || gWinMgr.isFullscreen) {
                    RECT rc;
                    gWinMgr.GetCenteredImageRect(&rc); // this rc should fully correspond to clip region

                    HRGN hRgn = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
                    SelectClipRgn(hdc, hRgn);

                    StretchBlt(hdc, rc.left - gWinMgr.x_poffset, rc.top - gWinMgr.y_poffset, gWinMgr.w_scaled, gWinMgr.h_scaled, pImage->hdc, 0, 0, width, height, SRCCOPY);
                } else {

                    RECT rc;
                    GetClientRect(gWinMgr.hWnd, &rc);

                    HRGN hRgn = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
                    SelectClipRgn(hdc, hRgn);

                    //StretchBlt(hdc, 0, 0, gWinMgr.w_scaled, gWinMgr.h_scaled, pImage->hdc, gWinMgr.x_poffset, gWinMgr.y_poffset, width, height, SRCCOPY);
                    StretchBlt(hdc, -gWinMgr.x_poffset, -gWinMgr.y_poffset, gWinMgr.w_scaled, gWinMgr.h_scaled, pImage->hdc, 0, 0, width, height, SRCCOPY);
                }
                SelectObject(pImage->hdc, oldbmp);

                std::cout << "REDRAW FAST " << gWinMgr.isMovingOrSizing  << " " << !frame->image->IsSubimageLoaded(frame->curFrame) <<std::endl;
                gWinMgr.fastDrawDone = true;
            } else {

                /*HDC memDC = CreateCompatibleDC(hdc);

                RECT rc;
                GetClientRect(gWinMgr.hWnd, &rc);
                HRGN hRgn = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
                SelectClipRgn(memDC, hRgn);

                HBITMAP hbitmap = CreateCompatibleBitmap(hdc, width, height);
                HGDIOBJ oldbmp = SelectObject(memDC, hbitmap);

                Gdiplus::Graphics graphics(memDC);
                // It's possible to draw directly into hdc at this point without using memDC and compatible bitmap.
                // But using it helps with flickering and there's no noticable slowdown

                //graphics.SetInterpolationMode(Gdiplus::InterpolationModeLowQuality); // Should use LQ while deconding and HQ when finished
                graphics.SetInterpolationMode(Gdiplus::InterpolationModeBicubic);
                Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromHBITMAP(pImage->hBitmap, NULL);
                graphics.DrawImage(bitmap, 0, 0, gWinMgr.w_scaled, gWinMgr.h_scaled);

                delete bitmap;

                BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

                std::cout << "REDRAW SMOOTH" << std::endl;

                SelectObject(memDC, oldbmp);
                DeleteObject(memDC);
                */

                //RECT rc;
                //GetClientRect(gWinMgr.hWnd, &rc);
                //HRGN hRgn = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
                //SelectClipRgn(hdc, hRgn);

                Gdiplus::Graphics graphics(hdc);
                Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromHBITMAP(pImage->hBitmap, NULL);
                //graphics.SetInterpolationMode(Gdiplus::InterpolationModeLowQuality); // Should use LQ while deconding and HQ when finished
                graphics.SetInterpolationMode(Gdiplus::InterpolationModeBicubic);

                if (gWinMgr.isMaximized || gWinMgr.isFullscreen) {
                    RECT rc;
                    gWinMgr.GetCenteredImageRect(&rc); // this rc should fully correspond to clip region

                    HRGN hRgn = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
                    SelectClipRgn(hdc, hRgn);

                    graphics.DrawImage(bitmap, rc.left-gWinMgr.x_poffset, rc.top-gWinMgr.y_poffset, gWinMgr.w_scaled, gWinMgr.h_scaled);
                }
                else {
                    RECT rc;
                    GetClientRect(gWinMgr.hWnd, &rc);

                    HRGN hRgn = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
                    SelectClipRgn(hdc, hRgn);
                    
                    graphics.DrawImage(bitmap, -gWinMgr.x_poffset, -gWinMgr.y_poffset, gWinMgr.w_scaled, gWinMgr.h_scaled);
                }

                delete bitmap;

                std::cout << "REDRAW SMOOTH" << std::endl;
            }
        }
    }
}


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR lpCmdLine, INT iCmdShow)
{
    HWND                hWnd;
    MSG                 msg;
    WNDCLASS            wndClass;
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR           gdiplusToken;

    PWCHAR cmdLine = GetCommandLineW();
    int argc = 0;
    WCHAR** argv = CommandLineToArgvW(cmdLine, &argc);
    if (argc > 1) {
        playlist = new Playlist(argv[1]);
    }
    else
        return 0;

    gWinMgr.ReadOrigin();
    
    // Initialize GDI+.
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);


    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_D4SEE_ICON));
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.lpszMenuName = NULL;
    //wndClass.lpszMenuName = MAKEINTRESOURCE(IDC_D4SEE);;
    wndClass.lpszClassName = TEXT("D4See");

    RegisterClass(&wndClass);

    hWnd = CreateWindow(
        TEXT("D4See"),            // window class name
        TEXT("D4See"),            // window caption
        WS_OVERLAPPEDWINDOW,      // window style
        CW_USEDEFAULT,            // initial x position
        CW_USEDEFAULT,            // initial y position
        CW_USEDEFAULT,            // initial x size
        CW_USEDEFAULT,            // initial y size
        NULL,                     // parent window handle
        NULL,                     // window menu handle
        hInstance,                // program instance handle
        NULL);                    // creation parameters

    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_D4SEE_ICON));
    SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

    gWinMgr.hWnd = hWnd;
    gWinMgr._TouchSizeEventTimestamp();

    ShowWindow(hWnd, iCmdShow);
    UpdateWindow(hWnd);
    DragAcceptFiles(hWnd, true);

    toml::value configData = gWinMgr.ReadConfig();
    gWinMgr.RestoreConfigValues(configData);
    
    
    //HMENU submenu = CreatePopupMenu();
    //AppendMenuW(submenu, MF_STRING, 1001, L"submenu 1001");

    //HMENU mainmenu = CreatePopupMenu();
    //AppendMenuW(mainmenu, MF_STRING, 100, L"main 100");
    //AppendMenuW(mainmenu, MF_SEPARATOR, 0, NULL);
    //AppendMenuW(mainmenu, MF_STRING, 101, L"main 101");

    //AppendMenuW(mainmenu, MF_POPUP, (UINT)submenu, L"&submenu");

    //gWinMgr.GetWindowSize();

    //HDC hdc = GetWindowDC(hWnd);
    

    frame = new MemoryFrame(hWnd, playlist->Current()->filename, playlist->Current()->format);
    gWinMgr.SelectFrame(frame);

    auto prevTime(std::chrono::steady_clock::now());

    bool shouldShutdown = false;
    while (!shouldShutdown) {

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            switch (msg.message) {
                case WM_COMMAND: {
                    unsigned int wmId = LOWORD(msg.wParam);
                    gWinMgr.HandleMenuCommand(wmId);
                    break;
                }
                case WM_TIMER: {
                    gWinMgr.Redraw();
                    gWinMgr.StopTimer();
                    break;
                }
                case WM_FRAMEREADY: {
                    MemoryFrame* f = (MemoryFrame*)msg.wParam;
                    if (f == frame) {
                        //gWinMgr.newImagePending = true;
                        //RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
                        std::cout << "- WM_FRAMEREADY " << std::endl;
                        ClearWindowForFrame(hWnd, frame);
                    }
                    break;
                }
                case WM_DROPFILES: {
                    HDROP hDrop = (HDROP)msg.wParam;
                    int fnSize = DragQueryFileW(hDrop, 0, NULL, 0);//Get Buffer Size

                    std::wstring filename;
                    filename.resize(fnSize);

                    DragQueryFileW(hDrop, 0, &filename[0], fnSize+1);

                    DragFinish(hDrop);


                    delete playlist;
                    playlist = new Playlist(filename);

                    delete frame;
                    auto cur = playlist->Current();
                    frame = new MemoryFrame(hWnd, cur->filename, cur->format);
                    gWinMgr.SelectFrame(frame);
                    break;
                }
                case WM_KEYDOWN: {
                    TranslateMessage(&msg);
                    switch (msg.wParam) {
                        case VK_ESCAPE: {
                            PostQuitMessage(0);
                            break;
                        }
                        case VK_RETURN: {
                            gWinMgr.ToggleFullscreen();
                            break;
                        }
                        case VK_LEFT: {
                            gWinMgr.Pan(-60, 0);
                            break;
                        }
                        case VK_RIGHT: {
                            gWinMgr.Pan(60, 0);
                            break;
                        }
                        case VK_UP: {
                            gWinMgr.Pan(0, -60);
                            break;
                        }
                        case VK_DOWN: {
                            gWinMgr.Pan(0, 60);
                            break;
                        }                                    
                        case VK_PRIOR: {
                            PreviousImage(hWnd);
                            break;
                        }
                        case VK_NEXT: {
                            NextImage(hWnd);
                            break;
                        }
                    }
                    break;
                }

                case WM_QUIT:
                    shouldShutdown = true;
            }

            DispatchMessage(&msg);
        }
        using namespace std::literals;
        auto now(std::chrono::steady_clock::now());
        if (frame) {
            if (!frame->isAnimated) {
                if (frame->decoderBatchId != frame->drawId) {
                    std::cout << "batchIDs " << frame->drawId << " " << frame->decoderBatchId << std::endl;
                    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
                    frame->drawId = frame->decoderBatchId;
                }
            } else {

                
                auto delta = now - prevTime;
                

                if (frame->AdvanceAnimation(delta)) {
                    //std::cout << "Redrawing" << std::endl;
                    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
                }
            }
        }
        prevTime = now;

        std::this_thread::sleep_for(5ms);
    }
    gWinMgr.DumpConfigValues(configData);
    gWinMgr.WriteConfig(configData);
    delete playlist;
    GdiplusShutdown(gdiplusToken);
    return msg.wParam;
}  // WinMain


LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam)
{
    HDC          hdc;
    PAINTSTRUCT  ps;

    switch (message)
    {
    case WM_LBUTTONDOWN: {
        int& sxPos = gWinMgr.mouseX;
        int& syPos = gWinMgr.mouseY;
        sxPos = GET_X_LPARAM(lParam);
        syPos = GET_Y_LPARAM(lParam);
        return 0;
    }
    case WM_RBUTTONDOWN: {
        POINT p;
        GetCursorPos(&p);
        //p.x = GET_X_LPARAM(lParam);
        //p.y = GET_Y_LPARAM(lParam);
        gWinMgr.ShowPopupMenu(p);
        break;
    }
    case WM_MOUSEWHEEL: {
        int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (zDelta < 0) {
            NextImage(hWnd);
        }
        else if (zDelta > 0) {
            PreviousImage(hWnd);
        }
        break;
    }
    case WM_MOUSEMOVE: {
        int LMBDown = (wParam) & MK_LBUTTON;
        if (LMBDown) {
            int& sxPos = gWinMgr.mouseX;
            int& syPos = gWinMgr.mouseY;

            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);

            if (sxPos != -1) {
                int dx = xPos - sxPos;
                int dy = yPos - syPos;

                gWinMgr.Pan(-dx, -dy);
            }

            sxPos = xPos;
            syPos = yPos;   
        }
        return 0;
    }
    case WM_SIZE: {
        int eventType = (int)wParam;
        if (eventType == SIZE_MAXIMIZED) {
            gWinMgr.isMaximized = true;
        }
        else if (eventType == SIZE_RESTORED) {
            gWinMgr.isMaximized = false;
        }
        gWinMgr.isMovingOrSizing = false;
        gWinMgr.UpdateWindowSizeInfo();
        return 0;
    }
    case WM_ENTERSIZEMOVE: {
        gWinMgr.isMovingOrSizing = true;
        return 0;
    }
    case WM_EXITSIZEMOVE: {
        gWinMgr.isMovingOrSizing = false;
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
        if (!gWinMgr.WasGeneratingEvents()) {
            gWinMgr.UpdateOrigin();
            gWinMgr.WriteOrigin();
        }
        return 0;
    }
    case WM_PAINT: {
        std::cout << "WM_PAINT" << std::endl;
        hdc = BeginPaint(hWnd, &ps);
        OnPaint(hdc);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
} // WndProc

#ifdef _DEBUG
int wmain(int argc, WCHAR* argv[], WCHAR* envp[]) {
    // Set Properties -> Linker -> System -> SubSystem to Console

    // Calling the wWinMain function to start the GUI program
    // Parameters:
    // GetModuleHandle(NULL) - To get a handle to the current instance
    // NULL - Previous instance is not needed
    // NULL - Command line parameters are not needed
    // 1 - To show the window normally
    wWinMain(GetModuleHandle(NULL), NULL, NULL, 1);

    //system("pause");
    return 0;
}
#endif

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
