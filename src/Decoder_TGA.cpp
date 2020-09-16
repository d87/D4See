#include "D4See.h"
#include "ImageFormats.h"
#include "Decoder_TGA.h"

#include <algorithm>
#include <limits>

using namespace D4See;


bool TGADecoder::Open(const wchar_t* filename, ImageFormat format) {
	FILE* f;
	_wfopen_s(&f, filename, L"rb");
	if (!f) {
		//errorf("Could not Open file \"%s\"", m_filename);
		return false;
	}

	//if (!IsValid(f)) {
	//	LOG("Not a valid TGA file");
	//	return false;
	//}
	//spec.filedesc = f;


	

	m_tga = TGAOpenFd(f);
	if (!m_tga || m_tga->last != TGA_OK) {
		/* error handling goes here */
		return NULL;
	}

	// Next replicating steps from TGAReadImage

	TGAData* data = &m_data;
	data->flags = TGA_IMAGE_DATA | TGA_IMAGE_ID;// | TGA_RGB;

	if (TGAReadHeader(m_tga) != TGA_OK) {
		// Couldn't read header
		return NULL;
	}

	if ((data->flags & TGA_IMAGE_ID) && m_tga->hdr.id_len != 0) {
		if (TGAReadImageId(m_tga, &data->img_id) != TGA_OK) {
			// Couldn't read image id
			return NULL;
		}
	}
	else {
		data->flags &= ~TGA_IMAGE_ID;
	}

	if (TGA_IS_MAPPED(m_tga)) {
		if (!TGAReadColorMap(m_tga, &data->cmap, data->flags)) {
			data->flags &= ~TGA_COLOR_MAP;
			// Couldn't read color map
			return NULL;
		}
		else {
			data->flags |= TGA_COLOR_MAP;
		}
	}

	// Now ready to read scanlines

	//if (TGAReadScanlines(tga, data->img_data, 0, tga->hdr.height, data->flags) != tga->hdr.height) {
	//	data->flags &= ~TGA_IMAGE_DATA;
	//	TGA_ERROR(tga, tga->last);
	//}


	// Should probably limit the support to only 24 and 32 bit images
	spec.format = ImageFormat::TGA;
	spec.filedesc = f;
	spec.numChannels = m_tga->hdr.depth / 8;;
	spec.width = m_tga->hdr.width;;
	spec.height = m_tga->hdr.height;
	spec.rowPitch = spec.numChannels * spec.width;
	spec.size = static_cast<uint64_t>(spec.height) * spec.rowPitch;
	spec.flipRowOrder = false;

	return true;
}

bool TGADecoder::IsValid(FILE* f) {
	return true;
}


unsigned int TGADecoder::Read(int startLine, int numLines, uint8_t* pDst) {
	int linesRead = 0;
	TGAData* data = &m_data;

	linesRead = TGAReadScanlines(m_tga, pDst, startLine, numLines, data->flags);

	if (linesRead == 0) {
		//error
	}

	// BGR to RGB
	uint8_t tmp;
	uint8_t numChannels = spec.numChannels;
	for (int i = 0; i < spec.rowPitch; i += numChannels) {
		tmp = pDst[i];
		pDst[i] = pDst[i + 2];
		pDst[i + 2] = tmp;
	}

	spec.linesRead += linesRead;

	if (spec.linesRead >= spec.height) {
		spec.isFinished = true;
		Close();
	}

	return linesRead;
}

void TGADecoder::Close() {
	if (spec.filedesc) {
		TGAClose(m_tga);
		fclose(spec.filedesc);
		spec.filedesc = NULL;
	}
}

TGADecoder::~TGADecoder() {
	//Close();
}
