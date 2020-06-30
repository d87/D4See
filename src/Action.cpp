#include "Action.h"
#include <string>

#include "WindowManager.h"

int sxPos = 0;
int syPos = 0;
MomentumAnimation inertia;

std::unordered_map<std::string, Action> actionMap;

std::unordered_map<std::string, Action>& getActionMap() {
    return actionMap;
}

void PanOnBtnDown() {
    POINT p;
    GetCursorPos(&p);
    HWND hWnd = gWinMgr.hWnd;
    //ScreenToClient(hWnd, &p);
    SetCapture(hWnd);

    sxPos = p.x;
    syPos = p.y;
    inertia.Start();
}

void PanOnMouseMove() {
    POINT p;
    GetCursorPos(&p);
    HWND hWnd = gWinMgr.hWnd;
    //ScreenToClient(hWnd, &p);

    int xPos = p.x;
    int yPos = p.y;

    if (sxPos != -1) {
        int dx = xPos - sxPos;
        int dy = yPos - syPos;

        //LOG_DEBUG("MouseMove: {0},{1}", dx, dy);

        inertia.AddVelocity(dx, dy);

        gWinMgr.Pan(-dx, -dy);
    }

    sxPos = xPos;
    syPos = yPos;
}

void PanOnBtnUp() {
    inertia.AddVelocity(0, 0);
    inertia.CountAverage();
    ReleaseCapture();
    gWinMgr.animations["PanningMomentum"] = (Animation*)&inertia;
}

void ToggleFullscreenOnClick() {
    gWinMgr.ToggleFullscreen();
}

void CutCurrentFile() {
    std::wstring path = gWinMgr.playlist->Current()->path;
    if (CutCopyFile(path, DROPEFFECT_MOVE)) { // successful cut
        // Remove from playlist
        gWinMgr.playlist->EraseCurrent();
        gWinMgr.PreviousImage();
    }
}

void CopyCurrentFile() {
    std::wstring path = gWinMgr.playlist->Current()->path;
    CutCopyFile(path, DROPEFFECT_COPY);
}

void RecycleFile() {
    auto wpath = gWinMgr.playlist->Current()->filename;
    DeleteFileDialog(wpath, true);
}

void NukeFile() {
    auto wpath = gWinMgr.playlist->Current()->filename;
    DeleteFileDialog(wpath, true);
}

void Quit() {
    PostQuitMessage(0);
}

void NextImage() {
    gWinMgr.NextImage();
}

void PrevImage() {
    gWinMgr.PreviousImage();
}

void ToggleAlwaysOnTop() {
    gWinMgr.ToggleAlwaysOnTop();
}

void ToggleBorderless() {
    gWinMgr.ToggleBorderless();
}

void MoveToPlaylistEnd() {
    gWinMgr.playlist->Move(PlaylistPos::End, 0);
    gWinMgr.LoadImageFromPlaylist(0);
}

void MoveToPlaylistStart() {
    gWinMgr.playlist->Move(PlaylistPos::Start, 0);
    gWinMgr.LoadImageFromPlaylist(0);
}

void KbPanLeft() {
    gWinMgr.Pan(-60, 0);
}
void KbPanRight() {
    gWinMgr.Pan(60, 0);
}
void KbPanUp() {
    gWinMgr.Pan(0, -60);
}
void KbPanDown() {
    gWinMgr.Pan(0, 60);
}

void ShowMenu() {
    POINT p;
    GetCursorPos(&p);
    gWinMgr.ShowPopupMenu(p);
}

// ---------- ZOOM

void ZoomInToPoint() {
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(gWinMgr.hWnd, &p);
    gWinMgr.ManualZoomToPoint(+0.15f, 0.0, p.x, p.y);
}

void ZoomOutToPoint() {
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(gWinMgr.hWnd, &p);
    gWinMgr.ManualZoomToPoint(-0.15f, 0.0, p.x, p.y);
}

float zoomStartValue;
void MouseZoomStart() {
    POINT p;
    GetCursorPos(&p);
    HWND hWnd = gWinMgr.hWnd;
    //ScreenToClient(hWnd, &p);
    SetCapture(hWnd);

    zoomStartValue = gWinMgr.canvas.scale_effective;
    sxPos = p.x;
    syPos = p.y;
}

void MouseZoomUpdate() {
    POINT p;
    GetCursorPos(&p);
    HWND hWnd = gWinMgr.hWnd;
    //ScreenToClient(hWnd, &p);
    float xoffset = p.x - sxPos;

    POINT zp;
    zp.x = sxPos;
    zp.y = syPos;
    ScreenToClient(hWnd, &zp);
    gWinMgr.ManualZoomToPoint(0.0, zoomStartValue + xoffset/200, zp.x, zp.y);
}

void MouseZoomEnd() {
    ReleaseCapture();
}


void RegisterAction(std::string actionName, ActionType actionType, callback_function cbOnDown, callback_function cbOnUp = NULL, callback_function cbOnMouseMove = NULL) {
    Action newAction;
    newAction.actionType = actionType;
    newAction.cbOnDown = cbOnDown;
    newAction.cbOnUp = cbOnUp;
    newAction.cbOnMouseMove = cbOnMouseMove;
    actionMap[actionName] = newAction;
}

void RegisterActions() {
    RegisterAction("TOGGLEFULLSCREEN", ActionType::PRESS, ToggleFullscreenOnClick);
    RegisterAction("TOGGLEBORDERLESS", ActionType::PRESS, ToggleBorderless);
    RegisterAction("CUTFILE", ActionType::PRESS, CutCurrentFile);
    RegisterAction("COPYFILE", ActionType::PRESS, CopyCurrentFile);
    RegisterAction("DELETEFILE", ActionType::PRESS, RecycleFile);
    RegisterAction("NUKEFILE", ActionType::PRESS, NukeFile);
    RegisterAction("QUIT", ActionType::PRESS, Quit);
    RegisterAction("SHOWMENU", ActionType::PRESS, ShowMenu);
    RegisterAction("NEXTIMAGE", ActionType::PRESS, NextImage);
    RegisterAction("PREVIMAGE", ActionType::PRESS, PrevImage);
    RegisterAction("FIRSTIMAGE", ActionType::PRESS, MoveToPlaylistStart);
    RegisterAction("LASTIMAGE", ActionType::PRESS, MoveToPlaylistEnd);
    RegisterAction("ZOOMINPOINT", ActionType::PRESS, ZoomInToPoint);
    RegisterAction("ZOOMOUTPOINT", ActionType::PRESS, ZoomOutToPoint);
    RegisterAction("MOUSEZOOM", ActionType::MOUSEMOVE, MouseZoomStart, MouseZoomEnd, MouseZoomUpdate);
    RegisterAction("ALWAYSONTOP", ActionType::PRESS, ToggleAlwaysOnTop);
    RegisterAction("PANUP", ActionType::PRESS, KbPanUp);
    RegisterAction("PANDOWN", ActionType::PRESS, KbPanDown);
    RegisterAction("PANLEFT", ActionType::PRESS, KbPanLeft);
    RegisterAction("PANRIGHT", ActionType::PRESS, KbPanRight);

    RegisterAction("MOUSEPAN", ActionType::MOUSEMOVE, PanOnBtnDown, PanOnBtnUp, PanOnMouseMove);
}