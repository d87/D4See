#pragma once
#include "Decoder.h"
#include <mutex>
#include <jpeglib.h>
#include <jconfig.h>
#include <jmorecfg.h>
#pragma(lib, "jpeg.lib")
#pragma(lib, "turbojpeg.lib")

namespace D4See {
	class JPEGDecoder final : public Decoder {
	private:
		jpeg_decompress_struct cinfo;
		jpeg_error_mgr jerr;

		std::mutex mutex;
		//bool isBufferedMode = false;
	public:
		~JPEGDecoder();
		virtual bool open(const char* filename) override;
		virtual void close() override;
		virtual unsigned int read(int startLine, int numLines, uint8_t* pDst) override;
	};
}