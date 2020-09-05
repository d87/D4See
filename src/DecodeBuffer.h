#pragma once

#include <string>
#include <memory>
#undef min // oiio got macro conflicts with gdi
#undef max
//#include <OpenImageIO/imageio.h>
#include "Decoder.h"
#include "ImageFormats.h"

struct DecoderBatchReturns {
	int status;
	unsigned int subimage;
	unsigned int mipLevel;
	unsigned int startLine;
	unsigned int endLine;
};

class DecodeBuffer {
	public: 
		std::wstring filename;
		ImageFormat format;
		D4See::Decoder* decoder = nullptr;

		D4See::ImageSpec spec;
		//unsigned int xres = 0; // image resolution
		//unsigned int yres = 0;
		//unsigned int xstride = 0;
		//unsigned int channels = 0;
		std::vector<unsigned char> pixels; // where pixel data is getting stored
		bool decodingComplete = false;
		unsigned int currentScanline = 0; // Goes back to 0 on the next mip/subimage

		float frameDelay = 0.1; // Applicable to animated GIFs. Stores frame delay for the currently decoded subimage

		unsigned int numSubimages = 1;
		unsigned int curSubimage = 0;

		DecodeBuffer();
		~DecodeBuffer();
		DecodeBuffer(std::wstring filename, ImageFormat format);
		int Open(std::wstring filename, ImageFormat format);
		bool IsFullyLoaded();
		bool IsSubimageLoaded(int subimage);
		DecoderBatchReturns PartialLoad(unsigned int numBytes, bool fullLoadFirstMipLevel);
};