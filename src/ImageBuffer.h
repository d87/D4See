#pragma once

#include <string>
#include <memory>
#undef min // oiio got macro conflicts with gdi
#undef max
#include <OpenImageIO/imageio.h>

#include "ImageFormats.h"

struct DecoderBatchReturns {
	int status;
	unsigned int subimage;
	unsigned int mipLevel;
	unsigned int startLine;
	unsigned int endLine;
};

class ImageBuffer {
	public: 
		std::string filename;
		ImageFormat format;
		std::unique_ptr<OIIO::ImageInput> in;
		unsigned int xres = 0;
		unsigned int yres = 0;
		unsigned int xstride = 0;
		unsigned int channels = 0;
		std::vector<unsigned char> pixels;
		bool decodingComplete = false;
		unsigned int currentScanline = 0;

		bool shouldSeek = true;
		float frameDelay = 0.2; // Applicable to animated GIFs. Store frame delay for the currently decoded subimage

		bool isAnimated = false;
		unsigned int numSubimages = 1;
		unsigned int curSubimage = 0;
		unsigned int numMipLevels = 0;
		unsigned int curMipLevel;

		ImageBuffer();
		ImageBuffer(std::string filename, ImageFormat format);
		int Open(std::string filename, ImageFormat format);
		void FullLoad();
		bool IsFullyLoaded();
		bool IsSubimageLoaded(int subimage);
		DecoderBatchReturns PartialLoad(unsigned int numBytes, bool fullLoadFirstMipLevel);
};