#pragma once
#include "Decoder.h"
#include <mutex>
#include <jpeglib.h>
#include <jconfig.h>
#include <jmorecfg.h>
#include <setjmp.h>
#include <libexif/exif-data.h>
#include <libexif/exif-loader.h>
#ifdef _DEBUG
	#pragma comment(lib, "jpegd.lib")
	#pragma comment(lib, "turbojpegd.lib")
#else
	#pragma comment(lib, "jpeg.lib")
	#pragma comment(lib, "turbojpeg.lib")
#endif
#pragma comment(lib, "libexif.lib")

namespace D4See {

	

	class JPEGDecoder final : public Decoder {
	public:
		jpeg_decompress_struct cinfo;
		//jpeg_error_mgr jerr;

		struct my_error_mgr {
			struct jpeg_error_mgr pub;    /* "public" fields */
			jmp_buf setjmp_buffer;        /* for return to caller */
			JPEGDecoder* jpegdecoder;
		};

		typedef struct my_error_mgr* my_error_ptr;
		my_error_mgr jerr;

		bool m_isCMYK = false;
		std::vector<uint8_t> m_cmyk_buf;
	private:
		std::mutex mutex;
		//bool isBufferedMode = false;
	public:
		~JPEGDecoder();
		static bool IsValid(FILE* f);
		virtual bool Open(FILE* f, const wchar_t* filename, ImageFormat format) override;
		virtual void Close() override;
		virtual unsigned int Read(int startLine, int numLines, uint8_t* pDst) override;
	//private:
		
	};


	
}