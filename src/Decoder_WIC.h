#pragma once
#include <wincodec.h>
#include "Decoder.h"
#include <mutex>

namespace D4See {
	class WICDecoder final : public Decoder {
	private:
		// The factory pointer
		IWICImagingFactory* m_pIWICFactory = NULL;

		// Create a decoder
		IWICBitmapDecoder* m_pDecoder = NULL;

		// Frame of the image from the decoder
		IWICBitmapFrameDecode* pFrame = NULL;
		IWICMetadataQueryReader* pQueryReader = NULL;

		// Color converter fot the frame, exports encoded to RGBA
		IWICFormatConverter* m_pConvertedSourceBitmap = NULL;

		std::mutex mutex;
		//bool isBufferedMode = false;
	private:
		int m_frameIndex = -1;
		unsigned int m_uFrameDelay = 0;

	public:
		~WICDecoder();
		virtual bool Open(const wchar_t* filename, ImageFormat format) override;
		virtual void Close() override;
		virtual unsigned int Read(int startLine, int numLines, uint8_t* pDst) override;
		virtual bool SelectFrame(int frameIndex) override;
		virtual float GetCurrentFrameDelay() override;

		virtual bool IsDirectPassAvailable() override;
		virtual IWICBitmapSource* GetFrameBitmapSource() override;
		virtual void PrepareNextFrameBitmapSource() override;
	};
}