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

#include <d2d1.h>
#pragma comment (lib,"d2d1.lib")


#include <chrono>

#include "util.h"
#include "playlist.h"
#include "D4See.h"
#include "DecodeBuffer.h"
#include "ImageContainer.h"
#include "WindowManager.h"


WindowManager gWinMgr;

ID2D1Factory* pD2DFactory;
//ID2D1DCRenderTarget* pRenderTarget;
ID2D1HwndRenderTarget* pRenderTarget;


VOID OnPaint() //HDC hdc)
{
    using namespace std::chrono_literals;

    ImageContainer* frame = gWinMgr.frame;

    if (frame) {

        if (frame->threadState > 0) {

            frame->bitmap_mutex.lock();

            ImageFrame* pImage = frame->GetActiveSubimage();

            int width = pImage->width;
            int height = pImage->height;
          
            
            RECT rc;
            GetClientRect(gWinMgr.hWnd, &rc);

            D2D1_SIZE_U size = D2D1::SizeU(
                rc.right - rc.left,
                rc.bottom - rc.top
            );
            pRenderTarget->Resize(size);

            //RECT rc;
            // GetClientRect(gWinMgr.hWnd, &rc);
            gWinMgr.GetCenteredImageRect(&rc); // this rc should fully correspond to clip region

            // Border
            //rc.left += 2;
            //rc.right -= 2;
            //rc.top += 2;
            //rc.bottom -= 2;

            
                
            //pRenderTarget->BindDC(hdc, &rc);

            pRenderTarget->BeginDraw();
            //pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
            pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));

            // Paint a grid background.
            /*pRenderTarget->FillRectangle(
                D2D1::RectF(0.0f, 0.0f, renderTargetSize.width, renderTargetSize.height),
                m_pGridPatternBitmapBrush
                );*/

                // Retrieve the size of the bitmap.
                //D2D1_SIZE_F size = m_pBitmap->GetSize();

            D2D1_POINT_2F upperLeftCorner = D2D1::Point2F(0.0f, 0.0f); // bitmap pos
            //D2D1_POINT_2F upperLeftCorner = D2D1::Point2F(g_fsX, g_fsY); // bitmap pos

            // Draw a bitmap.

            D2D1_MATRIX_3X2_F scaling = D2D1::Matrix3x2F::Scale(
                D2D1::Size(gWinMgr.scale_effective, gWinMgr.scale_effective),
                D2D1::Point2F(0.0f, 0.0f)
            );
            D2D1_MATRIX_3X2_F translation = D2D1::Matrix3x2F::Translation(-gWinMgr.x_poffset, -gWinMgr.y_poffset);

            pRenderTarget->SetTransform(translation);

                

            pRenderTarget->DrawBitmap(
                pImage->pBitmap,
                D2D1::RectF(
                    (float)rc.left,
                    (float)rc.top,
                    (float)rc.right,
                    (float)rc.bottom
                )
            );
                

            HRESULT hr = pRenderTarget->EndDraw();

            frame->bitmap_mutex.unlock();

            
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

    //int defaultWindowStyle = WS_OVERLAPPEDWINDOW;

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

    UpdateWindow(hWnd);
    DragAcceptFiles(hWnd, true);

    toml::value configData = gWinMgr.ReadConfig();
    gWinMgr.RestoreConfigValues(configData);

    
    PWCHAR cmdLine = GetCommandLineW();
    int argc = 0;
    WCHAR** argv = CommandLineToArgvW(cmdLine, &argc);
    Playlist* playlist;

    if (argc > 1) {
        playlist = new Playlist(argv[1], gWinMgr.sortMethod);
    }
    else {
        auto exeDir = GetExecutableDir();
        std::filesystem::path file(L"Splash.png");
        auto default_image = exeDir / file;
        playlist = new Playlist(default_image.wstring(), gWinMgr.sortMethod);
    }

    gWinMgr.ReadOrigin();
    gWinMgr.SelectPlaylist(playlist);
    
    
    //HMENU submenu = CreatePopupMenu();
    //AppendMenuW(submenu, MF_STRING, 1001, L"submenu 1001");

    //HMENU mainmenu = CreatePopupMenu();
    //AppendMenuW(mainmenu, MF_STRING, 100, L"main 100");
    //AppendMenuW(mainmenu, MF_SEPARATOR, 0, NULL);
    //AppendMenuW(mainmenu, MF_STRING, 101, L"main 101");

    //AppendMenuW(mainmenu, MF_POPUP, (UINT)submenu, L"&submenu");

    //gWinMgr.GetWindowSize();

    //HDC hdc = GetWindowDC(hWnd);

    //gWinMgr.frame->threadFinished.wait();

    //HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &pD2DFactory);

    // Create a pixel format and initial its format
    // and alphaMode fields.
    D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(
        DXGI_FORMAT_B8G8R8A8_UNORM,
        D2D1_ALPHA_MODE_IGNORE
    );

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();
    props.pixelFormat = pixelFormat;
    props.type = D2D1_RENDER_TARGET_TYPE_HARDWARE;
    //props.type = D2D1_RENDER_TARGET_TYPE_SOFTWARE;

    // Create a Direct2D render target					
    //hr = pD2DFactory->CreateDCRenderTarget(&props, &pRenderTarget);
    D2D1_SIZE_U size = D2D1::SizeU(
        600,
        480
    );
    hr = pD2DFactory->CreateHwndRenderTarget(props, D2D1::HwndRenderTargetProperties(hWnd, size), &pRenderTarget);

    //auto image = gWinMgr.frame->GetActiveSubimage();
    //D2D1_SIZE_U bitmapSize;
    //bitmapSize.height = image->height;
    //bitmapSize.width = image->width;

    //D2D1_BITMAP_PROPERTIES bitmapProperties;
    //bitmapProperties.dpiX = 96;
    //bitmapProperties.dpiY = 96;
    //bitmapProperties.pixelFormat = D2D1::PixelFormat(
    //    DXGI_FORMAT_B8G8R8A8_UNORM,
    //    D2D1_ALPHA_MODE_IGNORE
    //);

    //hr = pRenderTarget->CreateBitmap(
    //    bitmapSize,
    //    image->pPixels,
    //    image->pitch,
    //    &bitmapProperties,
    //    &pBitmap
    //);
    
    gWinMgr.SelectImage(new ImageContainer(hWnd, playlist->Current()->path, playlist->Current()->format));

    ShowWindow(hWnd, iCmdShow);

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
                    unsigned int id = msg.wParam;
                    if (id == D4S_TIMER_HQREDRAW) {
                        gWinMgr.Redraw();
                        gWinMgr.StopTimer(id);
                    }
                    else if (id == D4S_PREFETCH_TIMEOUT) {
                        gWinMgr.DiscardPrefetch();
                        gWinMgr.StopTimer(id);
                        std::cout << "Prefetch dropped from memory" << std::endl;
                    }
                    break;
                }
                case WM_FRAMEREADY: {
                    ImageContainer* f = (ImageContainer*)msg.wParam;
                    if (f == gWinMgr.frame) {
                        //gWinMgr.newImagePending = true;
                        //RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
                        std::cout << "- WM_FRAMEREADY " << std::endl;
                        gWinMgr.ResizeForImage();
                        gWinMgr.Redraw(RDW_ERASE);
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

                    auto playlist = new Playlist(filename, gWinMgr.sortMethod);
                    gWinMgr.SelectPlaylist(playlist);

                    auto cur = playlist->Current();
                    gWinMgr.SelectImage(new ImageContainer(hWnd, cur->path, cur->format));

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
                            gWinMgr.PreviousImage();
                            break;
                        }
                        case VK_NEXT: {
                            gWinMgr.NextImage();
                            break;
                        }
                        case VK_TAB: {
                            gWinMgr.ToggleBorderless();
                            break;
                        }
                        case VK_END: {
                            gWinMgr.playlist->Move(PlaylistPos::End, 0);
                            gWinMgr.LoadImageFromPlaylist(0);
                            break;
                        }
                        case VK_HOME: {
                            gWinMgr.playlist->Move(PlaylistPos::Start, 0);
                            gWinMgr.LoadImageFromPlaylist(0);
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
        if (gWinMgr.frame) {
            ImageContainer* frame = gWinMgr.frame;
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
        int fwKeys = GET_KEYSTATE_WPARAM(wParam);
        int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        bool isCtrlDown = fwKeys & MK_CONTROL;
        if (isCtrlDown) {
            if (zDelta < 0) {
                gWinMgr.ManualZoom(-0.10f);
            }
            else if (zDelta > 0) {
                gWinMgr.ManualZoom(+0.10f);
            }
        } else {
            if (zDelta < 0) {
                gWinMgr.NextImage();
            }
            else if (zDelta > 0) {
                gWinMgr.PreviousImage();
            }
        }
        break;
    }
    case WM_NCHITTEST: {
        if (GetAsyncKeyState(VK_MENU)) {
            LRESULT hit = DefWindowProc(hWnd, message, wParam, lParam);
            if (hit == HTCLIENT) hit = HTCAPTION;
            return hit;
        }
        else {
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    }
    case WM_LBUTTONUP: {
        if (gWinMgr.isPanning) {
            gWinMgr.isPanning = false;
            gWinMgr.ScheduleRedraw(50);
        }
        return 0;
    }
    case WM_MOUSEMOVE: {
        int LMBDown = (wParam) & MK_LBUTTON;
        if (LMBDown) {
            if (!gWinMgr.isPanning) {
                gWinMgr.isPanning = true;
                gWinMgr.StopTimer(D4S_TIMER_HQREDRAW);
            }

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
            // Initially first WM_SIZE is handled before the first frame is loaded
            if (gWinMgr.isMaximized) {
                gWinMgr.isMaximized = false;
                gWinMgr.ResizeForImage();
            }
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
    case WM_DISPLAYCHANGE:        
    case WM_PAINT: {
        std::cout << "WM_PAINT" << std::endl;
        ValidateRect(hWnd, NULL);
        OnPaint();
        //hdc = BeginPaint(hWnd, &ps);
        //OnPaint(hdc);
        //EndPaint(hWnd, &ps);
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
