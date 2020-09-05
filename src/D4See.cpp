//#include <stdafx.h>
#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX

#include <windows.h>
#include <windowsx.h>
#include <objidl.h>
#include <shellapi.h>

#include <locale>
#include <codecvt>
#include <chrono>

#include "util.h"
#include "D4See.h"
#include "playlist.h"
#include "DecodeBuffer.h"
#include "ImageContainer.h"
#include "WindowManager.h"
#include "Animation.h"
#include "Action.h"


WindowManager gWinMgr; // globaled in WindowManager.h

ID2D1Factory* pD2DFactory;
//ID2D1DCRenderTarget* pRenderTarget;
ID2D1HwndRenderTarget* pRenderTarget;
IDWriteFactory* pDWriteFactory;


VOID OnPaint() //HDC hdc)
{
    using namespace std::chrono_literals;

    ImageContainer* frame = gWinMgr.frame;

    if (frame) {

        ThreadState thread_state = frame->thread_state;
        if (thread_state > ThreadState::Uninitialized ) {

            RECT crc;
            GetClientRect(gWinMgr.hWnd, &crc);

            D2D1_SIZE_U size = D2D1::SizeU(
                crc.right - crc.left,
                crc.bottom - crc.top
            );
            pRenderTarget->Resize(size);

            RECT rc;
            gWinMgr.GetCenteredImageRect(&rc); // this rc should fully correspond to clip region

            frame->bitmap_mutex.lock();
 
            if (thread_state != ThreadState::Error) {
                ImageFrame* pImage = frame->GetActiveSubimage();

                if (pImage) {                  

                    //pRenderTarget->BindDC(hdc, &rc);

                    pRenderTarget->BeginDraw();
                    //pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
                    pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));

                    // Draw a bitmap.
                    D2D1_MATRIX_3X2_F scaling = D2D1::Matrix3x2F::Scale(
                        D2D1::Size(gWinMgr.canvas.scale_effective, gWinMgr.canvas.scale_effective),
                        D2D1::Point2F(0.0f, 0.0f)
                    );
                    D2D1_MATRIX_3X2_F translation = D2D1::Matrix3x2F::Translation(-gWinMgr.canvas.x_poffset, -gWinMgr.canvas.y_poffset);

                    pRenderTarget->SetTransform(translation);

                    //int bb = (gWinMgr.isBorderless) ? gWinMgr.borderlessBorder : 0;
                    //pRenderTarget->PushAxisAlignedClip(
                    //    D2D1::RectF(
                    //        (float)crc.left + 2,
                    //        (float)crc.top + 2,
                    //        (float)crc.right - 2,
                    //        (float)crc.bottom - 2
                    //    ), D2D1_ANTIALIAS_MODE_ALIASED
                    //);

                    //pRenderTarget->PopAxisAlignedClip();

                    //int width = pImage->width;
                    //int height = pImage->height;

                    pRenderTarget->DrawBitmap(
                        pImage->pBitmap,
                        D2D1::RectF(
                            static_cast<FLOAT>(rc.left),
                            static_cast<FLOAT>(rc.top),
                            static_cast<FLOAT>(rc.right),
                            static_cast<FLOAT>(rc.bottom)
                        )
                    );
                }

                HRESULT hr = pRenderTarget->EndDraw();
            }
            else {
                ID2D1SolidColorBrush* pBlackBrush;
                HRESULT hr = pRenderTarget->CreateSolidColorBrush(
                    D2D1::ColorF(D2D1::ColorF::White),
                    &pBlackBrush
                );

                IDWriteTextFormat* pTextFormat;

                hr = pDWriteFactory->CreateTextFormat(
                    L"Calibri",                // Font family name.
                    NULL,                       // Font collection (NULL sets it to use the system font collection).
                    DWRITE_FONT_WEIGHT_REGULAR,
                    DWRITE_FONT_STYLE_NORMAL,
                    DWRITE_FONT_STRETCH_NORMAL,
                    24.0f,
                    L"en-us",
                    &pTextFormat
                );
                hr = pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                hr = pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

                pRenderTarget->BeginDraw();
                //pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
                pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));

                // Create a D2D rect that is the same size as the window.
                D2D1_RECT_F layoutRect = D2D1::RectF(
                    static_cast<FLOAT>(rc.top),
                    static_cast<FLOAT>(rc.left),
                    static_cast<FLOAT>(rc.right - rc.left),
                    static_cast<FLOAT>(rc.bottom - rc.top)
                );


                std::wstring wtext;
                std::string str = frame->thread_error;
                int mbSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), &wtext[0], 0);
                wtext.resize(mbSize);
                MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), &wtext[0], mbSize);
                //std::string u8str;
                //int mbSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.length(), &u8str[0], 0, NULL, nullptr);
                //u8str.resize(mbSize);
                //WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.length(), &u8str[0], mbSize, NULL, nullptr);

                //std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                //std::wstring wtext = converter.from_bytes(frame->thread_error);

                // Use the DrawText method of the D2D render target interface to draw.
                pRenderTarget->DrawText(
                    wtext.c_str(),        // The string to render.
                    wtext.length(),    // The string's length.
                    pTextFormat,    // The text format.
                    layoutRect,      // The region of the window where the text will be rendered.
                    pBlackBrush      // The brush used to draw the text.
                );

                hr = pRenderTarget->EndDraw();

                pTextFormat->Release();
                pBlackBrush->Release();
            }


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


    
#ifdef SPDLOG_DEBUG
    // create color multi threaded logger
    auto console = spdlog::stdout_color_mt("console");
    auto err_logger = spdlog::stderr_color_mt("stderr");
    spdlog::set_pattern("[%M:%S:%f][thread %t] %L: %v");
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug
    spdlog::set_default_logger(console);
#endif // SPDLOG_INFO


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
    Playlist* playlist = new Playlist();
    playlist->SetSortingMethod(gWinMgr.sortMethod);

    if (argc > 1) {
        if (!playlist->GeneratePlaylist(argv[1])) {
            return 0;
        }
    }
    else {
        auto exeDir = GetExecutableDir();
        std::filesystem::path file(L"Splash.png");
        auto default_image = exeDir / file;
        if (!playlist->GeneratePlaylist(default_image.wstring())) {
            return 0;
        }
    }

    gWinMgr.ReadOrigin();
    gWinMgr.SelectPlaylist(playlist);
        
    //HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &pD2DFactory);

    // Create a shared DirectWrite factory.
    if (SUCCEEDED(hr))
    {
        hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(&pDWriteFactory)
        );
    }

    // Create a pixel format and initial its format
    // and alphaMode fields.
    D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(
        DXGI_FORMAT_B8G8R8A8_UNORM,
        D2D1_ALPHA_MODE_IGNORE
    );

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();
    props.pixelFormat = pixelFormat;
    props.type = D2D1_RENDER_TARGET_TYPE_HARDWARE;
    props.dpiX = 0; props.dpiY = 0;
    //props.type = D2D1_RENDER_TARGET_TYPE_SOFTWARE;

    // Create a Direct2D render target					
    //hr = pD2DFactory->CreateDCRenderTarget(&props, &pRenderTarget);
    D2D1_SIZE_U size = D2D1::SizeU(
        600,
        480
    );
    hr = pD2DFactory->CreateHwndRenderTarget(props, D2D1::HwndRenderTargetProperties(hWnd, size), &pRenderTarget);
    
    gWinMgr.SelectImage(new ImageContainer(hWnd, playlist->Current()->path, playlist->Current()->format));
    gWinMgr.frame->threadInitFinished.wait();
    gWinMgr.ResizeForImage();

    ShowWindow(hWnd, iCmdShow);


    RegisterActions();

    gWinMgr.input.BindKey("CTRL-F", "TOGGLEFULLSCREEN");
    gWinMgr.input.BindKey("RETURN", "TOGGLEFULLSCREEN");
    gWinMgr.input.BindKey("MBUTTON3", "TOGGLEFULLSCREEN");
    gWinMgr.input.BindKey("TAB", "TOGGLEBORDERLESS");
    gWinMgr.input.BindKey("CTRL-A", "ALWAYSONTOP");

    gWinMgr.input.BindKey("Z", "MOUSEZOOM");
    gWinMgr.input.BindKey("MBUTTON1", "MOUSEPAN");
    gWinMgr.input.BindKey("MBUTTON2", "SHOWMENU");
    gWinMgr.input.BindKey("CTRL-X", "CUTFILE");
    gWinMgr.input.BindKey("CTRL-C", "COPYFILE");
    gWinMgr.input.BindKey("DEL", "DELETEFILE");
    gWinMgr.input.BindKey("SHIFT-DEL", "NUKEFILE");
    gWinMgr.input.BindKey("ESC", "QUIT");
    
    gWinMgr.input.BindKey("SPACE", "NEXTIMAGE");
    gWinMgr.input.BindKey("PAGEDOWN", "NEXTIMAGE");
    gWinMgr.input.BindKey("MWHEELDOWN", "NEXTIMAGE");
    gWinMgr.input.BindKey("PAGEUP", "PREVIMAGE");
    gWinMgr.input.BindKey("MWHEELUP", "PREVIMAGE");
    gWinMgr.input.BindKey("HOME", "FIRSTIMAGE");
    gWinMgr.input.BindKey("END", "LASTIMAGE");

    
    gWinMgr.input.BindKey("CTRL-MWHEELUP", "ZOOMINPOINT");
    gWinMgr.input.BindKey("CTRL-MWHEELDOWN", "ZOOMOUTPOINT");

    gWinMgr.input.BindKey("F5", "REFRESHPLAYLIST");

    gWinMgr.input.BindKey("UP", "PANUP");
    gWinMgr.input.BindKey("DOWN", "PANDOWN");
    gWinMgr.input.BindKey("LEFT", "PANLEFT");
    gWinMgr.input.BindKey("RIGHT", "PANRIGHT");


    //IsExtensionAssociated(".png1231541", "D4See.png");


    using namespace std::chrono_literals;
    std::chrono::duration<float> elapsed = 0ms;

    auto prevTime(std::chrono::steady_clock::now());

    bool shouldShutdown = false;
    while (!shouldShutdown) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

            gWinMgr.input.ProcessInput(hWnd, msg.message, msg.wParam, msg.lParam);

            switch (msg.message) {
                case WM_COMMAND: {
                    unsigned int wmId = LOWORD(msg.wParam);
                    gWinMgr.HandleMenuCommand(wmId);
                    break;
                }
                case WM_TIMER: {
                    unsigned int id = msg.wParam;
                    if (id == D4S_PREFETCH_TIMEOUT) {
                        gWinMgr.DiscardPrefetch();
                        gWinMgr.StopTimer(id);
                        LOG("Prefetch dropped from memory");
                    }
                    break;
                }
                case WM_FRAMEERROR:
                case WM_FRAMEREADY: {
                    ImageContainer* f = (ImageContainer*)msg.wParam;
                    if (f == gWinMgr.frame) {
                        //gWinMgr.newImagePending = true;
                        //RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
                        LOG("Accepted WM_FRAMEREADY for {0}", wide_to_utf8(f->filename));
                        gWinMgr.ResizeForImage();
                        gWinMgr.Redraw(RDW_ERASE);
                    }
                    else {
                        LOG("Discarded WM_FRAMEREADY");
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
               

                case WM_QUIT:
                    shouldShutdown = true;
            }

            DispatchMessage(&msg);
        }
        using namespace std::literals;
        auto now(std::chrono::steady_clock::now());
        auto delta = now - prevTime;

        if (gWinMgr.frame) {
            ImageContainer* frame = gWinMgr.frame;
            if (!frame->isAnimated) {
                if (frame->decoderBatchId != frame->drawId) {
                    LOG("Batch Ids: {0} - {1}", frame->drawId, frame->decoderBatchId);
                    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
                    frame->drawId = frame->decoderBatchId;
                }
            } else {

                bool advanced = frame->AdvanceAnimation(delta);
                if (advanced) {
                    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
                }
                
            }

            elapsed += delta;
            if (elapsed > 16ms) {
                elapsed -= 16ms;
                auto it = gWinMgr.animations.begin();

                const std::string* deletionKey = nullptr;
                while (it != gWinMgr.animations.end()) {
                    if (it->second->Animate(gWinMgr.canvas, 16ms)) {
                        gWinMgr.LimitPanOffset();
                        gWinMgr.Redraw();
                    }
                    else {
                        deletionKey = &it->first;
                    }
                    it++;
                }

                if (deletionKey) {
                    gWinMgr.animations.erase(*deletionKey);
                }

            }
        }
        prevTime = now;

        std::this_thread::sleep_for(5ms);
    }
    gWinMgr.DumpConfigValues(configData);
    gWinMgr.WriteConfig(configData);

    pDWriteFactory->Release();
    pRenderTarget->Release();
    pD2DFactory->Release();

    return msg.wParam;
}  // WinMain


LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam)
{
    HDC          hdc;
    PAINTSTRUCT  ps;

    switch (message)
    {    
    //case WM_MOUSEWHEEL: {
    //    int fwKeys = GET_KEYSTATE_WPARAM(wParam);
    //    int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    //    bool isCtrlDown = fwKeys & MK_CONTROL;
    //    if (isCtrlDown) {
    //        POINT p;
    //        GetCursorPos(&p);
    //        ScreenToClient(hWnd, &p);
    //        if (zDelta < 0) {
    //            gWinMgr.ManualZoomToPoint(-0.15f, p.x, p.y);
    //        }
    //        else if (zDelta > 0) {
    //            gWinMgr.ManualZoomToPoint(+0.15f, p.x, p.y);
    //        }
    //    } else {
    //        if (zDelta < 0) {
    //            gWinMgr.NextImage();
    //        }
    //        else if (zDelta > 0) {
    //            gWinMgr.PreviousImage();
    //        }
    //    }
    //    break;
    //}
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
        LOG_TRACE("WM_PAINT");
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

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_ASSOCIATE_ALL: {
                    AssociateAllTypes();
                    break;
                }
                case IDC_ASSOCIATE_NONE: {
                    RemoveAssociations();
                    break;
                }
                case IDC_CHECK_FT_PNG: {
                    switch (HIWORD(wParam))
                    {
                        case BN_CLICKED: {
                            if (SendDlgItemMessage(hDlg, IDC_CHECK_FT_PNG, BM_GETCHECK, 0, 0))
                                MessageBox(NULL, "Checkbox Selected", "Success", MB_OK | MB_ICONINFORMATION);
                            else
                                MessageBox(NULL, "Checkbox Unselected", "Success", MB_OK | MB_ICONINFORMATION);
                        }
                    }
                    break;
                }
                case IDOK:
                case IDCANCEL: {
                    EndDialog(hDlg, LOWORD(wParam));
                    return (INT_PTR)TRUE;
                }
            }
            break;
        }
    }
    return (INT_PTR)FALSE;
}
