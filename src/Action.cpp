#include "Action.h"
#include <string>
#include <memory>

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
    auto wpath = gWinMgr.playlist->Current()->path;
    DeleteFileDialog(wpath, true);
}

void NukeFile() {
    auto wpath = gWinMgr.playlist->Current()->path;
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

void ToggleFitToScreen() {
    bool enabled = gWinMgr.shrinkToScreenWidth || gWinMgr.shrinkToScreenHeight || gWinMgr.stretchToScreenHeight || gWinMgr.stretchToScreenWidth;
    enabled = !enabled;
    gWinMgr.shrinkToScreenWidth = enabled;
    gWinMgr.shrinkToScreenHeight = enabled;
    gWinMgr.stretchToScreenHeight = enabled;
    gWinMgr.stretchToScreenWidth = enabled;

    gWinMgr.ResizeForImage();
    gWinMgr.Redraw(RDW_ERASE);
}

void ToggleZoomLock() {
    gWinMgr.zoomLock = !gWinMgr.zoomLock;
}

void RotateClockwise() {
    gWinMgr.canvas.CycleRotation(+1);

	gWinMgr.ResizeForImage();
	gWinMgr.Redraw(RDW_ERASE);
}

void RotateCounterClockwise() {
	gWinMgr.canvas.CycleRotation(-1);

	gWinMgr.ResizeForImage();
	gWinMgr.Redraw(RDW_ERASE);
}

void FlipHorizontal() {
    gWinMgr.canvas.flipHorizontal = !gWinMgr.canvas.flipHorizontal;

	gWinMgr.ResizeForImage();
	gWinMgr.Redraw(RDW_ERASE);
}

//void KbPanLeft() {
//    gWinMgr.Pan(-60, 0);
//}
//void KbPanRight() {
//    gWinMgr.Pan(60, 0);
//}
//void KbPanUp() {
//    gWinMgr.Pan(0, -60);
//}
//void KbPanDown() {
//    gWinMgr.Pan(0, 60);
//}

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

// ----------------

TranslateAnimation pos_horiz_anim(1500, 0);
void PosHPanStart() {
    gWinMgr.animations["HScroll"] = &pos_horiz_anim;
}
void PosHPanStop() {
    gWinMgr.animations.erase("HScroll");
}

TranslateAnimation neg_horiz_anim(-1500, 0);
void NegHPanStart() {
    gWinMgr.animations["HScroll"] = &neg_horiz_anim;
}
void NegHPanStop() {
    gWinMgr.animations.erase("HScroll");
}

TranslateAnimation pos_vert_anim(0, -1500);
void PosVPanStart() {
    gWinMgr.animations["VScroll"] = &pos_vert_anim;
}
void PosVPanStop() {
    gWinMgr.animations.erase("VScroll");
}

TranslateAnimation neg_vert_anim(0, 1500);
void NegVPanStart() {
    gWinMgr.animations["VScroll"] = &neg_vert_anim;
}
void NegVPanStop() {
    gWinMgr.animations.erase("VScroll");
}


void ThumbViewStart() {
    gWinMgr.canvas.scale_thumb = 0.5f;

	gWinMgr.ResizeForImage();
	gWinMgr.Redraw(RDW_ERASE);
}
void ThumbViewStop() {
    gWinMgr.canvas.scale_thumb = 1.0f;

	gWinMgr.ResizeForImage();
	gWinMgr.Redraw(RDW_ERASE);
}

void RefreshPlaylist() {
    gWinMgr.playlist->Refresh();
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
    RegisterAction("TOGGLESCREENFIT", ActionType::PRESS, ToggleFitToScreen);
    RegisterAction("ZOOMLOCK", ActionType::PRESS, ToggleZoomLock);
    RegisterAction("SHOWMENU", ActionType::PRESS, ShowMenu);
    RegisterAction("FLIPHORIZONTAL", ActionType::PRESS, FlipHorizontal);
    RegisterAction("ROTATECLOCKWISE", ActionType::PRESS, RotateClockwise);
    RegisterAction("ROTATECOUNTERCLOCKWISE", ActionType::PRESS, RotateCounterClockwise);
    RegisterAction("NEXTIMAGE", ActionType::PRESS, NextImage);
    RegisterAction("PREVIMAGE", ActionType::PRESS, PrevImage);
    RegisterAction("FIRSTIMAGE", ActionType::PRESS, MoveToPlaylistStart);
    RegisterAction("LASTIMAGE", ActionType::PRESS, MoveToPlaylistEnd);
    RegisterAction("ZOOMINPOINT", ActionType::PRESS, ZoomInToPoint);
    RegisterAction("ZOOMOUTPOINT", ActionType::PRESS, ZoomOutToPoint);
    RegisterAction("MOUSEZOOM", ActionType::MOUSEMOVE, MouseZoomStart, MouseZoomEnd, MouseZoomUpdate);
    RegisterAction("ALWAYSONTOP", ActionType::PRESS, ToggleAlwaysOnTop);
    RegisterAction("PANUP", ActionType::HOLD, PosVPanStart, PosVPanStop);
    RegisterAction("PANDOWN", ActionType::HOLD, NegVPanStart, NegVPanStop);
    RegisterAction("PANLEFT", ActionType::HOLD, NegHPanStart, NegHPanStop);
    RegisterAction("PANRIGHT", ActionType::HOLD, PosHPanStart, PosHPanStop);
    RegisterAction("THUMBNAILVIEW", ActionType::HOLD, ThumbViewStart, ThumbViewStop);

    RegisterAction("REFRESHPLAYLIST", ActionType::PRESS, RefreshPlaylist);

    RegisterAction("MOUSEPAN", ActionType::MOUSEMOVE, PanOnBtnDown, PanOnBtnUp, PanOnMouseMove);
}