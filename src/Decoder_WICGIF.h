#pragma once
#include <wincodec.h>
#include "Decoder.h"
#include <mutex>

namespace D4See {
	/*
	WIC GIF Decoder is using WIC to decode gifs and their metadata, but the big difference from WEBP and normal WIC decoder
	is that it's doing its own compositing to a bitmap render target, and that render target then gets snapshotted
	by D4See as a fully rasterized bitmap.
	*/
	class WICGIFDecoder final : public Decoder {
	private:
		// The factory pointer
		IWICImagingFactory* m_pIWICFactory = NULL;

		// Create a decoder
		IWICBitmapDecoder* m_pDecoder = NULL;

		// Frame of the image from the decoder
		IWICBitmapFrameDecode* pFrame = NULL;
		IWICMetadataQueryReader* pQueryReader = NULL;

		std::mutex mutex;
		//bool isBufferedMode = false;
	private:
		enum DISPOSAL_METHODS
		{
			DM_UNDEFINED = 0,
			DM_NONE = 1,
			DM_BACKGROUND = 2,
			DM_PREVIOUS = 3
		};

		int m_frameIndex = -1;
		unsigned int m_uFrameDelay = 0;
		unsigned int m_uFrameDisposal;
		ID2D1BitmapRenderTarget* m_pFrameComposeRT;
		ID2D1Bitmap* m_pRawFrame;
		ID2D1Bitmap* m_pSavedFrame;          // The temporary bitmap used for disposal 3 method
		unsigned int m_numFrames = 1;
		unsigned int m_canvasWidth;
		unsigned int m_canvasHeight;
		D2D1_COLOR_F m_backgroundColor;
		unsigned int m_cxGifImagePixel;  // Width of the displayed image in pixel calculated using pixel aspect ratio
		unsigned int m_cyGifImagePixel;  // Height of the displayed image in pixel calculated using pixel aspect rati
		unsigned int m_uNextFrameIndex;
		unsigned int m_uTotalLoopCount;  // The number of loops for which the animation will be played
		unsigned int m_uLoopNumber;      // The current animation loop number (e.g. 1 when the animation is first played)
		bool            m_fHasLoop;         // Whether the gif has a loop
		D2D1_RECT_F     m_framePosition;

		inline bool IsLastFrame()
		{
			return (m_uNextFrameIndex == 0);
		}

		bool EndOfAnimation()
		{
			return m_fHasLoop && IsLastFrame() && m_uLoopNumber == m_uTotalLoopCount + 1;
		}

		HRESULT GetRawFrame(UINT uFrameIndex);
		HRESULT GetGlobalMetadata();
		HRESULT GetBackgroundColor(IWICMetadataQueryReader* pMetadataQueryReader);

		HRESULT ComposeNextFrame();
		HRESULT DisposeCurrentFrame();
		HRESULT OverlayNextFrame();

		HRESULT SaveComposedFrame();
		HRESULT RestoreSavedFrame();
		HRESULT ClearCurrentFrameArea();
	public:
		~WICGIFDecoder();
		virtual bool Open(FILE* f, const wchar_t* filename, ImageFormat format) override;
		virtual void Close() override;
		virtual unsigned int Read(int startLine, int numLines, uint8_t* pDst) override;
		virtual bool SelectFrame(int frameIndex) override;
		virtual float GetCurrentFrameDelay() override;
		
		virtual int GetDirectPassType() override;
		virtual ID2D1BitmapRenderTarget* GetFrameD2D1BitmapRT() override;
		virtual void PrepareNextFrameBitmapSource() override;
	};
}