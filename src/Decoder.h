#pragma once
#include "ImageFormats.h"
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
		bool flipRowOrder;
		uint8_t nchannels;
		ImageFormat format;
		FILE* filedesc;
		wchar_t* filename;
		unsigned int linesRead;
		unsigned int linesPerChunk;
		bool isFinished;
	};


	class Decoder {
    public:
		ImageSpec spec;

		virtual bool open(const char* filename) = NULL;
		virtual void close() = NULL;
		virtual unsigned int read(int startLine, int numLines, uint8_t* pDst) = NULL;
		virtual bool select_frame(int frameIndex) { return true; };
	};
}