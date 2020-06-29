#include "Bindings.h"

using namespace D4See;


void D4See::BindToggleFullscreen(int released, void*pararms){
	if (released) return;
	gWinMgr.ToggleFullscreen();
}

//void BindAnimation(int released, void*pararms){
//	if (!released) return;
//	Animation->SetAcceleration(200.0);
//	Animation->SetVelocity(256.0,128.0);
//	Animation->Play();
//}
//
//void BindExit(int released, void*pararms){
//	if (!released) return;
//	PostQuitMessage( 0 );
//}
//void BindNext(int released, void*pararms){
//	if (released) return;
//	NextInPlaylist();
//}
//void BindPrev(int released, void*pararms){
//	if (released) return;
//	PrevInPlaylist();
//}
//void BindToStart(int released, void*pararms){
//	if (released) return;
//	if (plHead){
//		plCurrent = plHead;
//		PushBitmap(plCurrent->filename);
//	}
//}
//void BindToEnd(int released, void*pararms){
//	if (released) return;
//	if (plCurrent){
//		while (plCurrent->next) plCurrent = plCurrent->next;
//		PushBitmap(plCurrent->filename);
//	}
//}
//				
//void BindToggleAlwaysOnTop(int released, void*pararms){
//	if (released) return;
//	//isAlwaysOnTop = !isAlwaysOnTop;
//	//SetWindowPos(g_hWnd,isAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
//}
//void BindScrollRight(int released, void*pararms){
//	if (released) return;
//	Camera->Translate(-10.0, 0.0, 0.0);
//	Renderer->Render();
//}
//void BindScrollLeft(int released, void*pararms){
//	if (released) return;
//	Camera->Translate(10.0, 0.0, 0.0);
//	Renderer->Render();
//}
//void BindScrollUp(int released, void*pararms){
//	if (released) return;
//	Camera->Translate(0.0, -10.0, 0.0);
//	Renderer->Render();
//}
//void BindScrollDown(int released, void*pararms){
//	if (released) return;
//	Camera->Translate(0.0, 10.0, 0.0);
//	Renderer->Render();
//}
//
//void BindOpenFileDir(int released, void*pararms){
//	if (!released) return;
//	//OpenDir(initialFileDir);
//}
//
//
//void BindZooming(int released, void*pararms){
//	if (released){
//		ReleaseCapture();
//		IsZooming = 0;
//	}
//	else{
//		SetCapture(Renderer->hWnd);
//		IsZooming = 1;
//	}
//}
//
//void BindDragBtn(int released, void*pararms){
//	if (released)
//		Input->StopDragging();
//	else
//		Input->StartDragging();
//}
//void BindScrollBtn(int released, void*pararms){
//	if (released){
//		ReleaseCapture();
//		IsScrolling = 0;
//#ifdef SMOOTH_MOVE
//		Animation->SetAcceleration(5.0);
//
//		float Vx = 0.0;
//		float Vy = 0.0;
//		float dt = 0.0;
//		for (int i=0;i<LASTPOLLS;i++){
//			printf("==TABLES==\n");
//			printf("%d %f %f %f\n", i, VxArray[i], VyArray[i], dtArray[i]);
//			Vx+=VxArray[i];
//			Vy+=VyArray[i];
//			dt+=dtArray[i];
//		}
//		if (dt > 0 && (Vx != 0 || Vy != 0) ){
//			Vx/=dt; Vy/=dt;
//			Animation->SetVelocity(Vx *20,Vy *20);
//			Animation->Play();
//		}
//#endif
//	}
//	else{
//		SetCapture(Renderer->hWnd);
//		IsScrolling = 1;
//#ifdef SMOOTH_MOVE
//		Animation->Stop();
//		t2 = clock();
//		t1 = t2;
//		memset(VxArray,0,sizeof(float)*LASTPOLLS);
//		memset(VyArray,0,sizeof(float)*LASTPOLLS);
//		memset(dtArray,0,sizeof(float)*LASTPOLLS);
//#endif
//	}
//}
//void BindWheel(int released, void*pararms){
//	if (released)PrevInPlaylist();
//	else  NextInPlaylist();
//}
//
//void BindMouseMove(long dX, long dY){
//	if (IsZooming){
//		Camera->Zoom( float(dX)/Renderer->w_client*Camera->SCALE ); // 
//		Renderer->Render();
//	}
//	else if (IsScrolling){
//#ifdef SMOOTH_MOVE
//		t1 = t2;
//		t2 = clock()/CLOCKS_PER_SEC;
//		float dt = t2-t1;
//		printf("%f", dt);
//		if (dt > 0){
//			VxArray[VArrayIndex] = float(dX) / dt;
//			VyArray[VArrayIndex] = float(-dY) / dt;
//			dtArray[VArrayIndex] = dt;
//			if (VArrayIndex == LASTPOLLS) VArrayIndex = 0;
//			else VArrayIndex++;
//		}
//#endif
//		Camera->Translate( (float)dX/Camera->SCALE, (float)-dY/Camera->SCALE, 0.0f);
//		Renderer->Render();
//	}
//	else if (Input->IsDragging()){
//			Input->dragData.Xo+=dX;
//			Input->dragData.Yo+=dY;
//			RECT rc;
//			GetWindowRect(Renderer->hWnd,&rc);
//			SetWindowPos(Renderer->hWnd,0,
//					rc.left + Input->dragData.Xo,
//					rc.top + Input->dragData.Yo,
//					0,0,
//					SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);
//	}
//}