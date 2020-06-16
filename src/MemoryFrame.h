#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <thread>
#include <chrono>
#include "ImageBuffer.h"
#include "ImageFormats.h"


typedef struct
{
	int width;
	int height;
	int pitch;
	std::chrono::duration<float> frameDelay;
	HDC hdc;
	HBITMAP hBitmap;
	BITMAPINFO info;
	BYTE* pPixels;
} DIBImage;

class MemoryFrame {
	public:
		std::string filename;
		ImageFormat format;
		HWND hWnd; // HWND of the main window to send it messages
		HDC memDC; // Place where we'll paint
		std::mutex memDCmutex;

		std::vector<DIBImage> DIBitmap;
		unsigned int curFrame = 0; // index of currently active frame
		std::chrono::duration<float> frameTimeElapsed; // Animation : time elapsed showing activeframe

		std::atomic<int> subimagesReady = 0; // Amount of frames fully decoded into DIBs.
								// Should be the the upper limit for animation while decoding is still in progress

		//Gdiplus::Bitmap *DIBitmap; // ARGB 8bpp GDI+ bitmap, that is doing the painting
		ImageBuffer *image; // Abstraction of OIIO file decoding
		bool isAnimated = false;

		std::atomic<int> threadState{0};

		int drawId = 0;
		std::atomic<int> decoderBatchId{0};
		
		std::promise<bool> threadInitPromise;
		std::promise<bool> threadPromise;
	
		bool threadStarted;
		std::future<bool> threadInitFinished;
		std::future<bool> threadFinished;
	private:
		std::thread decoderThread;
	
	public:
		MemoryFrame(HWND hWnd);
		MemoryFrame(HWND hWnd, std::wstring filename, ImageFormat format);
		~MemoryFrame();
		void _Init(HWND hWnd);
		void OpenFile(std::wstring filename, ImageFormat format);
		bool AdvanceAnimation(std::chrono::duration<float> elapsed);
		bool PrevSubimage();
		bool NextSubimage();
		DIBImage* GetActiveSubimage();
		void StartThread();
		void TerminateThread();
		bool IsFinished();
		//void DecodingWork(MemoryFrame self);
};