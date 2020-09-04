#pragma once

#include <windows.h>
#include <thread>
#include <list>
#include <chrono>
#include <d2d1.h>
#include "DecodeBuffer.h"
#include "ImageFormats.h"
#include <mutex>
#include <atomic>
#include <future>

using namespace std::chrono_literals;

enum class ThreadState {
	Uninitialized,
	Initialized,
	BatchReady,
	Done,
	Abort,
	Error
};

typedef struct
{
	int width;
	int height;
	int pitch; // stride
	std::chrono::duration<float> frameDelay; // If animated, delay before switching to the next subimage.
	//HDC hdc; // Memory DC
	//HBITMAP hBitmap; // HBITMAP to DIBSection that is selected into DC
	//BITMAPINFO info;
	ID2D1Bitmap* pBitmap;
	BYTE* pPixels; // Pixel Data for DIBSection
} ImageFrame;

class ImageContainer {
	public:
		std::string filename;
		ImageFormat format;
		HWND hWnd; // HWND of the main window to send it messages
		std::mutex bitmap_mutex;

		int width = 0;
		int height = 0;

		std::vector<ImageFrame> frame;
		int curFrame = -1; // index of currently active frame, -1 = no frames are ready yet
		unsigned int numSubimages = 1;
		std::chrono::duration<float> frameTimeElapsed; // Animation : time elapsed showing activeframe

		std::mutex counter_mutex;
		int subimagesReady = 0; // Amount of frames fully decoded into DIBs.
								// Should be the the upper limit for animation while decoding is still in progress

		DecodeBuffer *image; // Abstraction of OIIO file decoding
		bool isAnimated = false;

		std::atomic<ThreadState> thread_state { ThreadState::Uninitialized };
		std::string thread_error;

		int drawId = 0;
		std::atomic<int> decoderBatchId{0};
		
		std::promise<bool> threadInitPromise;
		std::future<bool> threadInitFinished;
	private:
		std::thread decoderThread;
	
	public:
		ImageContainer(HWND hWnd);
		ImageContainer(HWND hWnd, std::wstring filename, ImageFormat format);
		~ImageContainer();
		void _Init(HWND hWnd);
		void OpenFile(std::wstring filename, ImageFormat format);
		bool AdvanceAnimation(std::chrono::duration<float> elapsed);
		//bool PrevSubimage();
		bool IsSubimageReady(int si);
		bool NextSubimage();
		inline ImageFrame* GetActiveSubimage();
		void StartThread();
		void TerminateThread();
		bool IsFinished();
};