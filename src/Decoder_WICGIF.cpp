#include "D4See.h"
#include "util.h"
#include <Windows.h>
#include <algorithm>
#include "ImageFormats.h"
#include "Decoder_WICGIF.h"
#include <io.h>

using namespace D4See;


bool WICGIFDecoder::Open(FILE* f, const wchar_t* filename, ImageFormat format) {

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
			GENERIC_READ,                    // Desired Read access to the file
			WICDecodeMetadataCacheOnDemand,  // Cache metadata when needed
			&m_pDecoder                        // Pointer to the decoder
		);

		if (SUCCEEDED(hr))
		{
			hr = GetGlobalMetadata();

			if (SUCCEEDED(hr)) {
				spec.format = format;
				spec.filedesc = f;
				spec.numChannels = 4;
				spec.numFrames = m_numFrames;
				spec.isAnimated = (m_numFrames > 1);
				spec.width = m_cxGifImagePixel;
				spec.height = m_cyGifImagePixel;
				spec.rowPitch = spec.numChannels * spec.width;
				spec.size = static_cast<uint64_t>(spec.height) * spec.rowPitch;
				spec.flipRowOrder = false;
				bResult = true;

				hr = pRenderTarget->CreateCompatibleRenderTarget(
					D2D1::SizeF(
						static_cast<float>(m_cxGifImagePixel),
						static_cast<float>(m_cyGifImagePixel)),
					&m_pFrameComposeRT);

				if (SUCCEEDED(hr)) {
					ComposeNextFrame();
				}
			}
		}
	}

	return bResult;
}

bool WICGIFDecoder::SelectFrame(int frameIndex) {

	return false;
};

unsigned int WICGIFDecoder::Read(int startLine, int numLines, uint8_t* pDst) {
	return 0;
}

float WICGIFDecoder::GetCurrentFrameDelay() {
	return (float)m_uFrameDelay / 1000;
}

int WICGIFDecoder::GetDirectPassType() {
	return DP_D2D1BitmapRenderTarget;
}

ID2D1BitmapRenderTarget* WICGIFDecoder::GetFrameD2D1BitmapRT() {
	return m_pFrameComposeRT;
}

void WICGIFDecoder::PrepareNextFrameBitmapSource() {
	if (spec.isAnimated && !IsLastFrame()) {
		ComposeNextFrame();
	}
	else {
		spec.isFinished = true;
	}
}

void WICGIFDecoder::Close() {
	if (spec.filedesc) {
		fclose(spec.filedesc);
		spec.filedesc = NULL;
	}
	SafeRelease(m_pFrameComposeRT);
	SafeRelease(m_pRawFrame);
	SafeRelease(m_pSavedFrame);
	SafeRelease(pFrame);
	SafeRelease(m_pDecoder);
	SafeRelease(m_pIWICFactory);
}

WICGIFDecoder::~WICGIFDecoder() {
	Close();
}


/******************************************************************
*                                                                 *
*  WICGIFDecoder::GetRawFrame()                                         *
*                                                                 *
*  Decodes the current raw frame, retrieves its timing            *
*  information, disposal method, and frame dimension for          *
*  rendering.  Raw frame is the frame read directly from the gif  *
*  file without composing.                                        *
*                                                                 *
******************************************************************/

HRESULT WICGIFDecoder::GetRawFrame(UINT uFrameIndex)
{
	IWICFormatConverter* pConverter = nullptr;
	IWICBitmapFrameDecode* pWicFrame = nullptr;
	IWICMetadataQueryReader* pFrameMetadataQueryReader = nullptr;

	PROPVARIANT propValue;
	PropVariantInit(&propValue);

	// Retrieve the current frame
	HRESULT hr = m_pDecoder->GetFrame(uFrameIndex, &pWicFrame);
	if (SUCCEEDED(hr))
	{
		// Format convert to 32bppPBGRA which D2D expects
		hr = m_pIWICFactory->CreateFormatConverter(&pConverter);
	}

	if (SUCCEEDED(hr))
	{
		hr = pConverter->Initialize(
			pWicFrame,
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			nullptr,
			0.f,
			WICBitmapPaletteTypeCustom);
	}

	if (SUCCEEDED(hr))
	{
		// Create a D2DBitmap from IWICBitmapSource
		SafeRelease(m_pRawFrame);
		hr = pRenderTarget->CreateBitmapFromWicBitmap(
			pConverter,
			nullptr,
			&m_pRawFrame);
	}

	if (SUCCEEDED(hr))
	{
		// Get Metadata Query Reader from the frame
		hr = pWicFrame->GetMetadataQueryReader(&pFrameMetadataQueryReader);
	}

	// Get the Metadata for the current frame
	if (SUCCEEDED(hr))
	{
		hr = pFrameMetadataQueryReader->GetMetadataByName(L"/imgdesc/Left", &propValue);
		if (SUCCEEDED(hr))
		{
			hr = (propValue.vt == VT_UI2 ? S_OK : E_FAIL);
			if (SUCCEEDED(hr))
			{
				m_framePosition.left = static_cast<float>(propValue.uiVal);
			}
			PropVariantClear(&propValue);
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = pFrameMetadataQueryReader->GetMetadataByName(L"/imgdesc/Top", &propValue);
		if (SUCCEEDED(hr))
		{
			hr = (propValue.vt == VT_UI2 ? S_OK : E_FAIL);
			if (SUCCEEDED(hr))
			{
				m_framePosition.top = static_cast<float>(propValue.uiVal);
			}
			PropVariantClear(&propValue);
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = pFrameMetadataQueryReader->GetMetadataByName(L"/imgdesc/Width", &propValue);
		if (SUCCEEDED(hr))
		{
			hr = (propValue.vt == VT_UI2 ? S_OK : E_FAIL);
			if (SUCCEEDED(hr))
			{
				m_framePosition.right = static_cast<float>(propValue.uiVal)
					+ m_framePosition.left;
			}
			PropVariantClear(&propValue);
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = pFrameMetadataQueryReader->GetMetadataByName(L"/imgdesc/Height", &propValue);
		if (SUCCEEDED(hr))
		{
			hr = (propValue.vt == VT_UI2 ? S_OK : E_FAIL);
			if (SUCCEEDED(hr))
			{
				m_framePosition.bottom = static_cast<float>(propValue.uiVal)
					+ m_framePosition.top;
			}
			PropVariantClear(&propValue);
		}
	}

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

		//if (SUCCEEDED(hr))
		//{
		//	// Insert an artificial delay to ensure rendering for gif with very small
		//	// or 0 delay.  This delay number is picked to match with most browsers' 
		//	// gif display speed.
		//	//
		//	// This will defeat the purpose of using zero delay intermediate frames in 
		//	// order to preserve compatibility. If this is removed, the zero delay 
		//	// intermediate frames will not be visible.
		//	if (m_uFrameDelay < 90)
		//	{
		//		m_uFrameDelay = 90;
		//	}
		//}
	}

	if (SUCCEEDED(hr))
	{
		if (SUCCEEDED(pFrameMetadataQueryReader->GetMetadataByName(
			L"/grctlext/Disposal",
			&propValue)))
		{
			hr = (propValue.vt == VT_UI1) ? S_OK : E_FAIL;
			if (SUCCEEDED(hr))
			{
				m_uFrameDisposal = propValue.bVal;
			}
		}
		else
		{
			// Failed to get the disposal method, use default. Possibly a 
			// non-animated gif.
			m_uFrameDisposal = DM_UNDEFINED;
		}
	}

	PropVariantClear(&propValue);

	SafeRelease(pConverter);
	SafeRelease(pWicFrame);
	SafeRelease(pFrameMetadataQueryReader);

	return hr;
}


HRESULT WICGIFDecoder::GetBackgroundColor(IWICMetadataQueryReader* pMetadataQueryReader)
{
	DWORD dwBGColor;
	BYTE backgroundIndex = 0;
	WICColor rgColors[256];
	UINT cColorsCopied = 0;
	PROPVARIANT propVariant;
	PropVariantInit(&propVariant);
	IWICPalette* pWicPalette = nullptr;

	// If we have a global palette, get the palette and background color
	HRESULT hr = pMetadataQueryReader->GetMetadataByName(
		L"/logscrdesc/GlobalColorTableFlag",
		&propVariant);
	if (SUCCEEDED(hr))
	{
		hr = (propVariant.vt != VT_BOOL || !propVariant.boolVal) ? E_FAIL : S_OK;
		PropVariantClear(&propVariant);
	}

	if (SUCCEEDED(hr))
	{
		// Background color index
		hr = pMetadataQueryReader->GetMetadataByName(
			L"/logscrdesc/BackgroundColorIndex",
			&propVariant);
		if (SUCCEEDED(hr))
		{
			hr = (propVariant.vt != VT_UI1) ? E_FAIL : S_OK;
			if (SUCCEEDED(hr))
			{
				backgroundIndex = propVariant.bVal;
			}
			PropVariantClear(&propVariant);
		}
	}

	// Get the color from the palette
	if (SUCCEEDED(hr))
	{
		hr = m_pIWICFactory->CreatePalette(&pWicPalette);
	}

	if (SUCCEEDED(hr))
	{
		// Get the global palette
		hr = m_pDecoder->CopyPalette(pWicPalette);
	}

	if (SUCCEEDED(hr))
	{
		hr = pWicPalette->GetColors(
			ARRAYSIZE(rgColors),
			rgColors,
			&cColorsCopied);
	}

	if (SUCCEEDED(hr))
	{
		// Check whether background color is outside range 
		hr = (backgroundIndex >= cColorsCopied) ? E_FAIL : S_OK;
	}

	if (SUCCEEDED(hr))
	{
		// Get the color in ARGB format
		dwBGColor = rgColors[backgroundIndex];

		// The background color is in ARGB format, and we want to 
		// extract the alpha value and convert it to float
		float alpha = (dwBGColor >> 24) / 255.f;
		m_backgroundColor = D2D1::ColorF(dwBGColor, alpha);
	}

	SafeRelease(pWicPalette);
	return hr;
}


HRESULT WICGIFDecoder::GetGlobalMetadata()
{
	PROPVARIANT propValue;
	PropVariantInit(&propValue);
	IWICMetadataQueryReader* pMetadataQueryReader = nullptr;

	// Get the frame count
	HRESULT hr = m_pDecoder->GetFrameCount(&m_numFrames);
	if (SUCCEEDED(hr))
	{
		// Create a MetadataQueryReader from the decoder
		hr = m_pDecoder->GetMetadataQueryReader(
			&pMetadataQueryReader);
	}

	if (SUCCEEDED(hr))
	{
		// Get background color
		if (FAILED(GetBackgroundColor(pMetadataQueryReader)))
		{
			// Default to transparent if failed to get the color
			m_backgroundColor = D2D1::ColorF(0, 0.f);
		}
	}

	// Get global frame size
	if (SUCCEEDED(hr))
	{
		// Get width
		hr = pMetadataQueryReader->GetMetadataByName(
			L"/logscrdesc/Width",
			&propValue);
		if (SUCCEEDED(hr))
		{
			hr = (propValue.vt == VT_UI2 ? S_OK : E_FAIL);
			if (SUCCEEDED(hr))
			{
				m_canvasWidth = propValue.uiVal;
			}
			PropVariantClear(&propValue);
		}
	}

	if (SUCCEEDED(hr))
	{
		// Get height
		hr = pMetadataQueryReader->GetMetadataByName(
			L"/logscrdesc/Height",
			&propValue);
		if (SUCCEEDED(hr))
		{
			hr = (propValue.vt == VT_UI2 ? S_OK : E_FAIL);
			if (SUCCEEDED(hr))
			{
				m_canvasHeight = propValue.uiVal;
			}
			PropVariantClear(&propValue);
		}
	}

	if (SUCCEEDED(hr))
	{
		// Get pixel aspect ratio
		hr = pMetadataQueryReader->GetMetadataByName(
			L"/logscrdesc/PixelAspectRatio",
			&propValue);
		if (SUCCEEDED(hr))
		{
			hr = (propValue.vt == VT_UI1 ? S_OK : E_FAIL);
			if (SUCCEEDED(hr))
			{
				UINT uPixelAspRatio = propValue.bVal;

				if (uPixelAspRatio != 0)
				{
					// Need to calculate the ratio. The value in uPixelAspRatio 
					// allows specifying widest pixel 4:1 to the tallest pixel of 
					// 1:4 in increments of 1/64th
					float pixelAspRatio = (uPixelAspRatio + 15.f) / 64.f;

					// Calculate the image width and height in pixel based on the
					// pixel aspect ratio. Only shrink the image.
					if (pixelAspRatio > 1.f)
					{
						m_cxGifImagePixel = m_canvasWidth;
						m_cyGifImagePixel = static_cast<unsigned int>(m_canvasHeight / pixelAspRatio);
					}
					else
					{
						m_cxGifImagePixel = static_cast<unsigned int>(m_canvasWidth * pixelAspRatio);
						m_cyGifImagePixel = m_canvasHeight;
					}
				}
				else
				{
					// The value is 0, so its ratio is 1
					m_cxGifImagePixel = m_canvasWidth;
					m_cyGifImagePixel = m_canvasHeight;
				}
			}
			PropVariantClear(&propValue);
		}
	}

	// Get looping information
	if (SUCCEEDED(hr))
	{
		// First check to see if the application block in the Application Extension
		// contains "NETSCAPE2.0" and "ANIMEXTS1.0", which indicates the gif animation
		// has looping information associated with it.
		// 
		// If we fail to get the looping information, loop the animation infinitely.
		if (SUCCEEDED(pMetadataQueryReader->GetMetadataByName(
			L"/appext/application",
			&propValue)) &&
			propValue.vt == (VT_UI1 | VT_VECTOR) &&
			propValue.caub.cElems == 11 &&  // Length of the application block
			(!memcmp(propValue.caub.pElems, "NETSCAPE2.0", propValue.caub.cElems) ||
				!memcmp(propValue.caub.pElems, "ANIMEXTS1.0", propValue.caub.cElems)))
		{
			PropVariantClear(&propValue);

			hr = pMetadataQueryReader->GetMetadataByName(L"/appext/data", &propValue);
			if (SUCCEEDED(hr))
			{
				//  The data is in the following format:
				//  byte 0: extsize (must be > 1)
				//  byte 1: loopType (1 == animated gif)
				//  byte 2: loop count (least significant byte)
				//  byte 3: loop count (most significant byte)
				//  byte 4: set to zero
				if (propValue.vt == (VT_UI1 | VT_VECTOR) &&
					propValue.caub.cElems >= 4 &&
					propValue.caub.pElems[0] > 0 &&
					propValue.caub.pElems[1] == 1)
				{
					m_uTotalLoopCount = MAKEWORD(propValue.caub.pElems[2],
						propValue.caub.pElems[3]);

					// If the total loop count is not zero, we then have a loop count
					// If it is 0, then we repeat infinitely
					if (m_uTotalLoopCount != 0)
					{
						m_fHasLoop = true;
					}
				}
			}
		}
	}

	PropVariantClear(&propValue);
	SafeRelease(pMetadataQueryReader);
	return hr;
}

/******************************************************************
*                                                                 *
*  WICGIFDecoder::RestoreSavedFrame()                             *
*                                                                 *
*  Copys the saved frame to the frame in the bitmap render        *
*  target.                                                        *
*                                                                 *
******************************************************************/

HRESULT WICGIFDecoder::RestoreSavedFrame()
{
	HRESULT hr = S_OK;

	ID2D1Bitmap* pFrameToCopyTo = nullptr;

	hr = m_pSavedFrame ? S_OK : E_FAIL;

	if (SUCCEEDED(hr))
	{
		hr = m_pFrameComposeRT->GetBitmap(&pFrameToCopyTo);
	}

	if (SUCCEEDED(hr))
	{
		// Copy the whole bitmap
		hr = pFrameToCopyTo->CopyFromBitmap(nullptr, m_pSavedFrame, nullptr);
	}

	SafeRelease(pFrameToCopyTo);

	return hr;
}

/******************************************************************
*                                                                 *
*  WICGIFDecoder::ClearCurrentFrameArea()                         *
*                                                                 *
*  Clears a rectangular area equal to the area overlaid by the    *
*  current raw frame in the bitmap render target with background  *
*  color.                                                         *
*                                                                 *
******************************************************************/

HRESULT WICGIFDecoder::ClearCurrentFrameArea()
{
	m_pFrameComposeRT->BeginDraw();

	// Clip the render target to the size of the raw frame
	m_pFrameComposeRT->PushAxisAlignedClip(
		&m_framePosition,
		D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

	m_pFrameComposeRT->Clear(m_backgroundColor);

	// Remove the clipping
	m_pFrameComposeRT->PopAxisAlignedClip();

	return m_pFrameComposeRT->EndDraw();
}

/******************************************************************
*                                                                 *
*  WICGIFDecoder::DisposeCurrentFrame()                           *
*                                                                 *
*  At the end of each delay, disposes the current frame           *
*  based on the disposal method specified.                        *
*                                                                 *
******************************************************************/

HRESULT WICGIFDecoder::DisposeCurrentFrame()
{
	HRESULT hr = S_OK;

	switch (m_uFrameDisposal)
	{
	case DM_UNDEFINED:
	case DM_NONE:
		// We simply draw on the previous frames. Do nothing here.
		break;
	case DM_BACKGROUND:
		// Dispose background
		// Clear the area covered by the current raw frame with background color
		hr = ClearCurrentFrameArea();
		break;
	case DM_PREVIOUS:
		// Dispose previous
		// We restore the previous composed frame first
		hr = RestoreSavedFrame();
		break;
	default:
		// Invalid disposal method
		hr = E_FAIL;
	}

	return hr;
}

/******************************************************************
*                                                                 *
*  WICGIFDecoder::OverlayNextFrame()                              *
*                                                                 *
*  Loads and draws the next raw frame into the composed frame     *
*  render target. This is called after the current frame is       *
*  disposed.                                                      *
*                                                                 *
******************************************************************/

HRESULT WICGIFDecoder::OverlayNextFrame()
{
	// Get Frame information
	HRESULT hr = GetRawFrame(m_uNextFrameIndex);
	if (SUCCEEDED(hr))
	{
		// For disposal 3 method, we would want to save a copy of the current
		// composed frame
		if (m_uFrameDisposal == DM_PREVIOUS)
		{
			hr = SaveComposedFrame();
		}
	}

	if (SUCCEEDED(hr))
	{
		// Start producing the next bitmap
		m_pFrameComposeRT->BeginDraw();

		// If starting a new animation loop
		if (m_uNextFrameIndex == 0)
		{
			// Draw background and increase loop count
			m_pFrameComposeRT->Clear(m_backgroundColor);
			m_uLoopNumber++;
		}

		// Produce the next frame
		m_pFrameComposeRT->DrawBitmap(
			m_pRawFrame,
			m_framePosition);

		hr = m_pFrameComposeRT->EndDraw();
	}

	// To improve performance and avoid decoding/composing this frame in the 
	// following animation loops, the composed frame can be cached here in system 
	// or video memory.

	if (SUCCEEDED(hr))
	{
		// Increase the frame index by 1
		m_uNextFrameIndex = (++m_uNextFrameIndex) % m_numFrames;
	}

	return hr;
}

/******************************************************************
*                                                                 *
*  WICGIFDecoder::SaveComposedFrame()                             *
*                                                                 *
*  Saves the current composed frame in the bitmap render target   *
*  into a temporary bitmap. Initializes the temporary bitmap if   *
*  needed.                                                        *
*                                                                 *
******************************************************************/

HRESULT WICGIFDecoder::SaveComposedFrame()
{
	HRESULT hr = S_OK;

	ID2D1Bitmap* pFrameToBeSaved = nullptr;

	hr = m_pFrameComposeRT->GetBitmap(&pFrameToBeSaved);
	if (SUCCEEDED(hr))
	{
		// Create the temporary bitmap if it hasn't been created yet 
		if (m_pSavedFrame == nullptr)
		{
			auto bitmapSize = pFrameToBeSaved->GetPixelSize();
			D2D1_BITMAP_PROPERTIES bitmapProp;
			pFrameToBeSaved->GetDpi(&bitmapProp.dpiX, &bitmapProp.dpiY);
			bitmapProp.pixelFormat = pFrameToBeSaved->GetPixelFormat();

			hr = m_pFrameComposeRT->CreateBitmap(
				bitmapSize,
				bitmapProp,
				&m_pSavedFrame);
		}
	}

	if (SUCCEEDED(hr))
	{
		// Copy the whole bitmap
		hr = m_pSavedFrame->CopyFromBitmap(nullptr, pFrameToBeSaved, nullptr);
	}

	SafeRelease(pFrameToBeSaved);

	return hr;
}

/******************************************************************
*                                                                 *
*  WICGIFDecoder::ComposeNextFrame()                              *
*                                                                 *
*  Composes the next frame by first disposing the current frame   *
*  and then overlaying the next frame. More than one frame may    *
*  be processed in order to produce the next frame to be          *
*  displayed due to the use of zero delay intermediate frames.    *
*  Also, sets a timer that is equal to the delay of the frame.    *
*                                                                 *
******************************************************************/

HRESULT WICGIFDecoder::ComposeNextFrame()
{
	HRESULT hr = S_OK;

	// Check to see if the render targets are initialized
	if (m_pFrameComposeRT)
	{
		// First, kill the timer since the delay is no longer valid
		//KillTimer(m_hWnd, DELAY_TIMER_ID);

		// Compose one frame
		hr = DisposeCurrentFrame();
		if (SUCCEEDED(hr))
		{
			hr = OverlayNextFrame();
		}

		// Keep composing frames until we see a frame with delay greater than
		// 0 (0 delay frames are the invisible intermediate frames), or until
		// we have reached the very last frame.
		while (SUCCEEDED(hr) && m_uFrameDelay == 0 && !IsLastFrame())
		{
			hr = DisposeCurrentFrame();
			if (SUCCEEDED(hr))
			{
				hr = OverlayNextFrame();
			}
		}

		// If we have more frames to play, set the timer according to the delay.
		// Set the timer regardless of whether we succeeded in composing a frame
		// to try our best to continue displaying the animation.
		if (!EndOfAnimation() && m_numFrames > 1)
		{
			// Set the timer according to the delay
			//SetTimer(m_hWnd, DELAY_TIMER_ID, m_uFrameDelay, nullptr);
		} else {
			spec.isFinished = true;
		}
	}

	return hr;
}