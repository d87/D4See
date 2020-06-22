#include "DecodeBuffer.h"
#include "D4See.h"
#include <algorithm>

DecodeBuffer::DecodeBuffer() {
}

DecodeBuffer::DecodeBuffer(std::string filename, ImageFormat format) {
	Open(filename, format);
}

int DecodeBuffer::Open(std::string filename, ImageFormat format) {
	this->format = format;
	
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	in = OIIO::ImageInput::open(filename);
	if (!in) {
		std::string error = OIIO::geterror();
		throw std::runtime_error(error);
	}
	const OIIO::ImageSpec& spec = in->spec();

	xres = spec.width;
	yres = spec.height;
	xstride = xres * spec.nchannels;
	channels = spec.nchannels;
	unsigned long size = yres * xres * channels;
	pixels.resize(size);


	
	if (format == ImageFormat::GIF) {
		// Some animated gifs don't have either of these params.

		OIIO::TypeDesc typedesc;
		typedesc = spec.getattributetype("oiio:Movie");
		int isMovie = 0;
		spec.getattribute("oiio:Movie", typedesc, &isMovie);
		isAnimated = isMovie == 1;


		typedesc = spec.getattributetype("FramesPerSecond");
		int fps[2];
		if (spec.getattribute("FramesPerSecond", typedesc, &fps)) {
			frameDelay = float(fps[1]) / fps[0];
			isAnimated = true;
		}
	}
	
	

	int mip = 0;
	while (in->seek_subimage(0, mip)) {
		mip++;
	}

	// Seeking through the whole file just to get the amount of frames is very slow, for gifs at least

	//int subi = 1;
	//while (in->seek_subimage(subi, 0)) {
	//	subi++;
	//}
	//numSubimages = subi;

	numMipLevels = mip;
	curMipLevel = mip - 1;

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	LOG("Opened {0} in {1}ms", filename, std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());

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

	const OIIO::ImageSpec& spec = in->spec();

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