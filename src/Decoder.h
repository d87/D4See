#pragma once
#include "ImageFormats.h"
#include "D4See.h"
#include <d2d1.h>
#include <stdint.h>
#include <vector>
#include <string>

namespace D4See {
	struct ImageSpec {
		uint8_t* pData;
		uint8_t* pCursor;
		uint32_t width;
		uint32_t height;
		uint64_t size;
		uint32_t rowPitch;
		uint32_t rowPadding;
		uint32_t internalRotation;
		bool flipRowOrder;
		uint8_t numChannels;
		uint32_t numFrames;
		bool isAnimated;
		ImageFormat format;
		FILE* filedesc;
		wchar_t* filename;
		unsigned int linesRead;
		unsigned int linesPerChunk; // unused?
		bool isFinished;
	};
	
	class Decoder {

	protected:
		enum {
			DP_NONE,
			DP_D2D1BitmapRenderTarget,
		};
    public:
		ImageSpec spec;
		virtual bool Open(const wchar_t* filename, ImageFormat format);
		virtual bool Open(FILE* f, const wchar_t* filename, ImageFormat format) = NULL;
		virtual void Close() = NULL;
		virtual unsigned int Read(int startLine, int numLines, uint8_t* pDst) = NULL;
		virtual bool SelectFrame(int frameIndex) { return true; };
		virtual float GetCurrentFrameDelay() { return 0; };

		virtual int GetDirectPassType() { return DP_NONE; };
		virtual ID2D1BitmapRenderTarget* GetFrameD2D1BitmapRT() { return nullptr; };
		virtual void PrepareNextFrameBitmapSource() {};
	};
}