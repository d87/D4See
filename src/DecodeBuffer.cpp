#include "DecodeBuffer.h"
#include "Decoder.h"
#include "Decoder_JPEG.h"
#include "Decoder_WIC.h"
#include "D4See.h"
#include "util.h"
#include <algorithm>
#include <chrono>
#include <stdexcept>

DecodeBuffer::DecodeBuffer() {
}

DecodeBuffer::~DecodeBuffer() {
	if (decoder) {
		decoder->close();
		delete decoder;
	}
}

DecodeBuffer::DecodeBuffer(std::wstring filename, ImageFormat format) {
	Open(filename, format);
}

int DecodeBuffer::Open(std::wstring filename, ImageFormat format) {
	this->format = format;
	
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	if (format == ImageFormat::JPEG) {
		decoder = new D4See::JPEGDecoder();
	}
	else {
		decoder = new D4See::WICDecoder();
	}

	if (!decoder) {
		return 0;
	}

	if (!decoder->open(filename.c_str())) {
		throw std::runtime_error("Couldn't open decoder");
	}

	auto spec = decoder->spec;

	//in = OIIO::ImageInput::open(filename);
	//if (!in) {
	//	std::string error = OIIO::geterror();
	//	throw std::runtime_error(error);
	//}
	//const OIIO::ImageSpec& spec = in->spec();

	xres = spec.width;
	yres = spec.height;
	xstride = xres * spec.nchannels;
	channels = spec.nchannels;
	unsigned long size = yres * xres * channels;
	pixels.resize(size);


	//
	//if (format == ImageFormat::GIF) {
	//	// Some animated gifs don't have either of these params.

	//	OIIO::TypeDesc typedesc;
	//	typedesc = spec.getattributetype("oiio:Movie");
	//	int isMovie = 0;
	//	spec.getattribute("oiio:Movie", typedesc, &isMovie);
	//	isAnimated = isMovie == 1;


	//	typedesc = spec.getattributetype("FramesPerSecond");
	//	int fps[2];
	//	if (spec.getattribute("FramesPerSecond", typedesc, &fps)) {
	//		frameDelay = float(fps[1]) / fps[0];
	//		isAnimated = true;
	//	}
	//}
	//
	

	int mip = 0;
	//while (in->seek_subimage(0, mip)) {
	//	mip++;
	//}

	// Seeking through the whole file just to get the amount of frames is very slow, for gifs at least

	//int subi = 1;
	//while (in->seek_subimage(subi, 0)) {
	//	subi++;
	//}
	//numSubimages = subi;

	numMipLevels = mip;
	curMipLevel = mip - 1;

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	LOG("Opened {0} in {1}ms", wide_to_utf8(filename), std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());

	return 1;
}


bool DecodeBuffer::IsSubimageLoaded(int subimage) {
	return curSubimage > subimage;
}

bool DecodeBuffer::IsFullyLoaded() {
	return decodingComplete;
}


DecoderBatchReturns DecodeBuffer::PartialLoad(unsigned int numBytes, bool fullLoadFirstMipLevel) {
	if (IsFullyLoaded())
		return { 0, 0, 0, 0, 0 };

	auto spec = decoder->spec;

	//if (shouldSeek) {
	//	in->seek_subimage(curSubimage, curMipLevel);
	//	if (isAnimated) {
	//		OIIO::TypeDesc typedesc = spec.getattributetype("FramesPerSecond");
	//		int fps[2];
	//		if (spec.getattribute("FramesPerSecond", typedesc, &fps)) {
	//			frameDelay = float(fps[1]) / fps[0];
	//		}
	//	}
	//}


	int reqScanlines = numBytes / xstride + 1;

	unsigned long n = currentScanline * xstride;
	unsigned char* startingPoint = &pixels[n];
	unsigned char* cursor = startingPoint;

	unsigned int yStart = currentScanline;

	if (fullLoadFirstMipLevel) {
		//if ((curMipLevel == numMipLevels - 1) && numMipLevels > 1)
		//	reqScanlines = std::min((unsigned int)1200, yres);
	}

	//int completed = 0;
	//for (completed = 0; completed < reqScanlines && currentScanline < yres; completed++) {
	//	//in->read_scanline(currentScanline, 0, OIIO::TypeDesc::UINT8, cursor);
	//	decoder->read(currentScanline, 1, cursor);
	//	currentScanline++;
	//	cursor += xstride;
	//}
	int completed = decoder->read(currentScanline, 1, cursor);
	currentScanline += completed;
	cursor += completed*xstride;

	unsigned int yEnd = yStart + completed;

	unsigned int cSI = curSubimage;
	unsigned int cML = curMipLevel;

	// This way avoids unnecessary seeking, which is causing slowdown on gifs
	// 

	if (currentScanline == yres) {
		curSubimage++;
		if (decoder->spec.isFinished) {
			decodingComplete = true;
			decoder->close();
		}
		else {
			curMipLevel++;
			currentScanline = 0;
		}

		return { 2, cSI, cML, yStart, yEnd };
	}

	return { 1, cSI, cML, yStart, yEnd };
}

/*
DecoderBatchReturns DecodeBuffer::PartialLoad(unsigned int numBytes, bool fullLoadFirstMipLevel) {
	if (IsFullyLoaded())
		return { 0, 0, 0, 0, 0 };

	//const OIIO::ImageSpec& spec = in->spec();
	auto spec = decoder->spec;

	//if (shouldSeek) {
	//	in->seek_subimage(curSubimage, curMipLevel);
	//	if (isAnimated) {
	//		OIIO::TypeDesc typedesc = spec.getattributetype("FramesPerSecond");
	//		int fps[2];
	//		if (spec.getattribute("FramesPerSecond", typedesc, &fps)) {
	//			frameDelay = float(fps[1]) / fps[0];
	//		}
	//	}
	//}

	if (spec.tile_width == 0) {

		int reqScanlines = numBytes / xstride + 1;

		unsigned long n = currentScanline * xstride;
		unsigned char* startingPoint = &pixels[n];
		unsigned char* cursor = startingPoint;

		unsigned int yStart = currentScanline;

		if (fullLoadFirstMipLevel) {
			if ((curMipLevel == numMipLevels - 1) && numMipLevels > 1)
				reqScanlines = std::min((unsigned int)1200, yres);
		}

		int completed = 0;
		for (completed = 0; completed < reqScanlines && currentScanline < yres; completed++) {
			in->read_scanline(currentScanline, 0, OIIO::TypeDesc::UINT8, cursor);
			currentScanline++;
			cursor += xstride;
		}

		unsigned int yEnd = yStart + completed;

		unsigned int cSI = curSubimage;
		unsigned int cML = curMipLevel;

		// This way avoids unnecessary seeking, which is causing slowdown on gifs
		// 

		if (currentScanline == yres) {
			if (curMipLevel == 0) {
				if (in->seek_subimage(curSubimage+1, numMipLevels-1)) { // try to open next subimage
					curMipLevel = numMipLevels - 1;
					currentScanline = 0;
					curSubimage++;
					numSubimages = curSubimage+1;
					if (isAnimated) {
						OIIO::TypeDesc typedesc = spec.getattributetype("FramesPerSecond");
						int fps[2];
						if (spec.getattribute("FramesPerSecond", typedesc, &fps)) {
							frameDelay = float(fps[1]) / fps[0];
						}
					}
					//shouldSeek = true;
				} else {
					curSubimage++;
					decodingComplete = true;
					in->close();
				}
			}
			else {
				curMipLevel--;
				currentScanline = 0;
				in->seek_subimage(curSubimage, curMipLevel);
				//shouldSeek = true;
			}

			return { 2, cSI, cML, yStart, yEnd };
		}

		return { 1, cSI, cML, yStart, yEnd };
	}
	else {
		throw std::runtime_error("Not handling tiled files");;
	}
}
*/