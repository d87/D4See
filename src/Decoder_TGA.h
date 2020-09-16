#pragma once
#include "Decoder.h"
#include <mutex>

#include "tga.h"
#pragma comment(lib, "libtga.lib")

namespace D4See {

	class TGADecoder final : public Decoder {
	public:

	private:
		TGA* m_tga = nullptr;
		TGAData m_data;
	public:
		~TGADecoder();
		virtual bool Open(const wchar_t* filename, ImageFormat format) override;
		virtual void Close() override;
		virtual unsigned int Read(int startLine, int numLines, uint8_t* pDst) override;

		bool IsValid(FILE* f);

	//private:
		
	};


	
}