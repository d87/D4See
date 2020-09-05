#include "D4See.h"
#include "util.h"
#include <Windows.h>
#include "ImageFormats.h"
#include "Decoder_WIC.h"

using namespace D4See;

bool WICDecoder::open(const wchar_t* filename) {

	

	FILE* f;
	_wfopen_s(&f, filename, L"rb");
	if (!f) {
		LOG("Could not open file \"%s\"", wide_to_utf8(filename));
		return false;
	}
	
	spec.filedesc = f;

	// Initialize COM
	CoInitialize(NULL);

	//mutex.lock();

	bool bResult = false;

	// Create the COM imaging factory
	HRESULT hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&m_pIWICFactory)
	);

	if (SUCCEEDED(hr)) {
		hr = m_pIWICFactory->CreateDecoderFromFilename(
			filename,                      // Image to be decoded
			NULL,                            // Do not prefer a particular vendor
			GENERIC_READ,                    // Desired read access to the file
			WICDecodeMetadataCacheOnDemand,  // Cache metadata when needed
			&m_pDecoder                        // Pointer to the decoder
		);

		if (select_frame(0)) {
			unsigned int width;
			unsigned int height;
			pFrame->GetSize(&width, &height);
			spec.format = ImageFormat::PNG;
			spec.filedesc = f;
			spec.nchannels = 4;
			spec.width = width;
			spec.height = height;
			spec.rowPitch = spec.nchannels * spec.width;
			spec.size = static_cast<uint64_t>(spec.height) * spec.rowPitch;
			spec.flipRowOrder = false;
			bResult = true;
		}
	}

	//mutex.unlock();
	return bResult;
}

bool WICDecoder::select_frame(int frameIndex) {
	if (pFrame) pFrame->Release();
	HRESULT hr = m_pDecoder->GetFrame(frameIndex, &pFrame);
	if (SUCCEEDED(hr))
	{
		//Step 3: Format convert the frame to 32bppPBGRA
		if (SUCCEEDED(hr))
		{
			if (m_pConvertedSourceBitmap) m_pConvertedSourceBitmap->Release();
			hr = m_pIWICFactory->CreateFormatConverter(&m_pConvertedSourceBitmap);
			
			if (SUCCEEDED(hr))
			{
				hr = m_pConvertedSourceBitmap->Initialize(
					pFrame,                          // Input bitmap to convert
					GUID_WICPixelFormat32bppPRGBA,   // Destination pixel format
					WICBitmapDitherTypeNone,         // Specified dither pattern
					nullptr,                         // Specify a particular palette 
					0.f,                             // Alpha threshold
					WICBitmapPaletteTypeCustom       // Palette translation type
				);


				// m_pConvertedSourceBitmap->CopyPixels(...)

				return true;
			}
		}
	}
	return false;
};

unsigned int WICDecoder::read(int startLine, int numLines, uint8_t* pDst) {
	WICRect rc;
	rc.X = 0;
	rc.Y = startLine;
	rc.Width = spec.width;
	rc.Height = numLines;
	uint32_t size = numLines * spec.rowPitch;
	HRESULT hr = m_pConvertedSourceBitmap->CopyPixels(&rc, spec.rowPitch, size, pDst);
	if (SUCCEEDED(hr))
	{
		spec.linesRead += numLines;
		if (spec.linesRead == spec.height) {
			spec.isFinished = true;
		}
		return numLines;
	}
	return 0;
}

template <typename T>
inline void SafeRelease(T*& p)
{
	if (nullptr != p)
	{
		p->Release();
		p = nullptr;
	}
}

void WICDecoder::close() {
	if (spec.filedesc) {
		fclose(spec.filedesc);
		spec.filedesc = NULL;
	}
	SafeRelease(m_pConvertedSourceBitmap);
	SafeRelease(pFrame);
	SafeRelease(m_pDecoder);
	SafeRelease(m_pIWICFactory);
}

WICDecoder::~WICDecoder() {
	close();
}