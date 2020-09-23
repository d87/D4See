#include "Decoder.h"
#include "D4See.h"
#include "util.h"
#include <vector>

using namespace D4See;

Decoder::Decoder() {
	memset(&spec, 0, sizeof(spec));
}

bool Decoder::Open(const wchar_t* filename, ImageFormat format) {
	FILE* f;
	_wfopen_s(&f, filename, L"rb");
	if (!f) {
		LOG("Could not open file \"%s\"", wide_to_utf8(filename));
		return false;
	}
	Open(f, filename, format);
}