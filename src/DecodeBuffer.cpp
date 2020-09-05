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
		throw std::runtime_error("Couldn't open decoder");
	}

	if (!decoder->open(filename.c_str(), format)) {
		throw std::runtime_error("Couldn't open decoder");
	}

	spec = decoder->spec;
	

	unsigned long size = spec.height * spec.width * spec.numChannels;
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


	int reqScanlines = numBytes / spec.rowPitch + 1;

	unsigned long n = currentScanline * spec.rowPitch;
	unsigned char* startingPoint = &pixels[n];
	unsigned char* cursor = startingPoint;

	unsigned int yStart = currentScanline;

	//int completed = 0;
	//for (completed = 0; completed < reqScanlines && currentScanline < yres; completed++) {
	//	//in->read_scanline(currentScanline, 0, OIIO::TypeDesc::UINT8, cursor);
	//	decoder->read(currentScanline, 1, cursor);
	//	currentScanline++;
	//	cursor += xstride;
	//}

	// Reading 1 scanline, because in practice it's the fastest
	int completed = decoder->read(currentScanline, 1, cursor);
	currentScanline += completed;
	cursor += completed * spec.rowPitch;

	unsigned int yEnd = yStart + completed;

	unsigned int cSI = curSubimage;
	unsigned int cML = 0;

	if (currentScanline == spec.height) {
		curSubimage++;
		if (decoder->spec.isFinished) {
			decodingComplete = true;
			decoder->close();
		}
		else {
			currentScanline = 0;
		}

		return { 2, cSI, cML, yStart, yEnd };
	}

	return { 1, cSI, cML, yStart, yEnd };
}
