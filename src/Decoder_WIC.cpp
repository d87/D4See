#include "D4See.h"
#include "util.h"
#include <Windows.h>
#include <algorithm>
#include "ImageFormats.h"
#include "Decoder_WIC.h"

using namespace D4See;

bool WICDecoder::open(const wchar_t* filename, ImageFormat format) {
	FILE* f;
	_wfopen_s(&f, filename, L"rb");
	if (!f) {
		LOG("Could not open file \"%s\"", wide_to_utf8(filename));
		return false;
	}
	
	spec.filedesc = f;

	// Initialize COM
	CoInitialize(NULL);

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

		if (SUCCEEDED(hr))
		{
			unsigned int frameCount;
			hr = m_pDecoder->GetFrameCount(&frameCount);
			if (FAILED(hr)) {
				frameCount = 1;
			}

			if (select_frame(0)) {
				unsigned int width;
				unsigned int height;
				pFrame->GetSize(&width, &height);
				spec.format = format;
				spec.filedesc = f;
				spec.numChannels = 4;
				spec.numFrames = frameCount;
				spec.isAnimated = (frameCount > 1);
				spec.width = width;
				spec.height = height;
				spec.rowPitch = spec.numChannels * spec.width;
				spec.size = static_cast<uint64_t>(spec.height) * spec.rowPitch;
				spec.flipRowOrder = false;
				bResult = true;
			}
		}
	}

	return bResult;
}

bool WICDecoder::select_frame(int frameIndex) {
	if (m_frameIndex == frameIndex)
		return true;

	m_frameIndex = frameIndex;

	SafeRelease(pFrame);
	HRESULT hr = m_pDecoder->GetFrame(frameIndex, &pFrame);
	if (SUCCEEDED(hr))
	{
		IWICMetadataQueryReader* pFrameMetadataQueryReader = nullptr;

		PROPVARIANT propValue;
		PropVariantInit(&propValue);

		if (spec.format == ImageFormat::GIF) {

			hr = pFrame->GetMetadataQueryReader(&pFrameMetadataQueryReader);

			if (SUCCEEDED(hr))
			{
				// Get delay from the optional Graphic Control Extension
				if (SUCCEEDED(pFrameMetadataQueryReader->GetMetadataByName(
					L"/grctlext/Delay",
					&propValue)))
				{
					hr = (propValue.vt == VT_UI2 ? S_OK : E_FAIL);
					if (SUCCEEDED(hr))
					{
						// Convert the delay retrieved in 10 ms units to a delay in 1 ms units
						hr = UIntMult(propValue.uiVal, 10, &m_uFrameDelay);
					}
					PropVariantClear(&propValue);
				}
				else
				{
					// Failed to get delay from graphic control extension. Possibly a
					// single frame image (non-animated gif)
					m_uFrameDelay = 0;
				}

			}
		}

		if (spec.format == ImageFormat::WEBP) {
			// WebP Animation metadata
			// / ANIM / LoopCount(on a container query reader, returns a PropVariant of type VT_UI2)
			// / ANMF / FrameDuration(on a frame query reader, returns a PropVariant of type VT_UI4)
			hr = pFrame->GetMetadataQueryReader(&pFrameMetadataQueryReader);

			if (SUCCEEDED(hr))
			{
				// Get delay from the optional Graphic Control Extension
				if (SUCCEEDED(pFrameMetadataQueryReader->GetMetadataByName(
					L"/ANMF/FrameDuration",
					&propValue)))
				{
					hr = (propValue.vt == VT_UI4 ? S_OK : E_FAIL);
					m_uFrameDelay = propValue.uiVal;
					//if (SUCCEEDED(hr))
					//{
					//	// Convert the delay retrieved in 10 ms units to a delay in 1 ms units
					//	hr = UIntMult(propValue.uiVal, 10, &m_uFrameDelay);
					//}
					PropVariantClear(&propValue);
				}
				else
				{
					// Failed to get delay from graphic control extension. Possibly a
					// single frame image (non-animated gif)
					m_uFrameDelay = 0;
				}

			}
		}


		
	}

	
	if (SUCCEEDED(hr))
	{
		// Format convert the frame to 32bppPRGBA
		SafeRelease(m_pConvertedSourceBitmap);
		if (!m_pConvertedSourceBitmap) {
			hr = m_pIWICFactory->CreateFormatConverter(&m_pConvertedSourceBitmap);
			if (FAILED(hr))
			{
				return false;
			}
		}

		hr = m_pConvertedSourceBitmap->Initialize(
			pFrame,                          // Input bitmap to convert
			GUID_WICPixelFormat32bppPRGBA,   // Destination pixel format
			WICBitmapDitherTypeNone,         // Specified dither pattern
			nullptr,                         // Specify a particular palette 
			0.5f,                             // Alpha threshold
			WICBitmapPaletteTypeCustom       // Palette translation type
		);

		if (SUCCEEDED(hr))
			return true;
	}
	return false;
};
#undef min
unsigned int WICDecoder::read(int startLine, int numLines, uint8_t* pDst) {
	WICRect rc;
	rc.X = 0;
	rc.Y = startLine;
	rc.Width = spec.width;
	//int remains = (spec.height - startLine);
	//numLines = std::min(remains, numLines);
	rc.Height = numLines;
	uint32_t size = numLines * spec.rowPitch;
	HRESULT hr = m_pConvertedSourceBitmap->CopyPixels(&rc, spec.rowPitch, size, pDst);
	if (SUCCEEDED(hr))
	{
		spec.linesRead += numLines;
		if (spec.linesRead == spec.height) {
			if (spec.isAnimated && m_frameIndex < spec.numFrames - 1) {
				select_frame(m_frameIndex + 1);
				spec.linesRead = 0;
			}
			else {
				spec.isFinished = true;
			}
		}
		return numLines;
	}
	return 0;
}

float WICDecoder::get_current_frame_delay() {
	return (float)m_uFrameDelay/1000;
}

bool WICDecoder::direct_pass_available() {
	if (spec.format == ImageFormat::GIF) {
		return true;
	}
	return false;
}

IWICBitmapSource* WICDecoder::get_direct_bitmap_source() {
	return m_pConvertedSourceBitmap;
}

void WICDecoder::prepare_next_bitmap_source() {
	if (spec.isAnimated && m_frameIndex < spec.numFrames - 1) {
		select_frame(m_frameIndex + 1);
	}
	else {
		spec.isFinished = true;
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