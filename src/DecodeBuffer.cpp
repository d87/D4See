#include "DecodeBuffer.h"
#include "Decoder.h"
#include "Decoder_JPEG.h"
#include "Decoder_TGA.h"
#include "Decoder_WIC.h"
#include "Decoder_WICGIF.h"
#include "D4See.h"
#include "util.h"
#include <algorithm>
#include <chrono>
#include <stdexcept>

DecodeBuffer::DecodeBuffer() {
}

DecodeBuffer::~DecodeBuffer() {
	if (decoder) {
		decoder->Close();
		delete decoder;
	}
}

DecodeBuffer::DecodeBuffer(std::wstring filename, ImageFormat format) {
	Open(filename, format);
}

int DecodeBuffer::Open(std::wstring filename, ImageFormat format) {

	FILE* f;
	_wfopen_s(&f, filename.c_str(), L"rb");
	if (!f) {
		LOG("Could not open file \"%s\"", wide_to_utf8(filename));
		throw std::runtime_error("Couldn't open decoder");
		return NULL;
	}

	if (format == ImageFormat::JPEG && !D4See::JPEGDecoder::IsValid(f))
		format = ImageFormat::UNKNOWN;


	this->format = format;
	
	switch (format)
	{
	case ImageFormat::JPEG:
		decoder = new D4See::JPEGDecoder();
		break;
	case ImageFormat::TGA:
		decoder = new D4See::TGADecoder();
		break;
	case ImageFormat::GIF:
		decoder = new D4See::WICGIFDecoder();
		break;
	default:
		decoder = new D4See::WICDecoder();
		break;
	}

	if (!decoder) {
		throw std::runtime_error("Couldn't open decoder");
	}

	if (!decoder->Open(f, filename.c_str(), format)) {
		throw std::runtime_error("Couldn't open decoder");
	}

	spec = decoder->spec;
	

	unsigned long size = spec.height * spec.width * spec.numChannels;
	pixels.resize(size);

	return 1;
}


bool DecodeBuffer::IsSubimageLoaded(int subimage) {
	return curSubimage > subimage;
}

bool DecodeBuffer::IsFullyLoaded() {
	return decoder->spec.isFinished;
}


DecoderBatchReturns DecodeBuffer::PartialLoad(unsigned int numBytes, bool fullLoadFirstMipLevel) {
	if (IsFullyLoaded())
		return { DecoderStatus::Finished, 0, 0, 0, 0 };

	auto spec = decoder->spec;

	int reqScanlines = numBytes / spec.rowPitch + 1;

	unsigned long n = currentScanline * spec.rowPitch;
	unsigned char* startingPoint = &pixels[n];
	unsigned char* cursor = startingPoint;

	unsigned int yStart = currentScanline;

	//int completed = 0;
	//for (completed = 0; completed < reqScanlines && currentScanline < yres; completed++) {
	//	//in->read_scanline(currentScanline, 0, OIIO::TypeDesc::UINT8, cursor);
	//	decoder->Read(currentScanline, 1, cursor);
	//	currentScanline++;
	//	cursor += xstride;
	//}

	// Reading 1 scanline, because in practice it's the fastest
	int completed = decoder->Read(currentScanline, 1, cursor);
	currentScanline += completed;
	cursor += completed * spec.rowPitch;

	unsigned int yEnd = yStart + completed;

	unsigned int cSI = curSubimage;
	unsigned int cML = 0;
	DecoderStatus status = InProgress;

	if (currentScanline == spec.height) {
		curSubimage++;
		if (decoder->spec.isFinished) {
			decodingComplete = true;
			decoder->Close();
			status = DecoderStatus::Finished;
		}
		else {
			currentScanline = 0;
			status = spec.isAnimated ? DecoderStatus::SubimageFinished : DecoderStatus::InProgress;
		}
	}

	return { status, cSI, cML, yStart, yEnd };
}
