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
		virtual bool open(const wchar_t* filename, ImageFormat format) override;
		virtual void close() override;
		virtual unsigned int read(int startLine, int numLines, uint8_t* pDst) override;
		virtual bool select_frame(int frameIndex) override;
		virtual float get_current_frame_delay() override;
	};
}