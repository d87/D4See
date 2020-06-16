#include "util.h"


std::string wide_to_utf8(std::wstring wstr) {
	std::string u8str;
	int mbSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.length(), &u8str[0], 0, NULL, nullptr);
	u8str.resize(mbSize);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.length(), &u8str[0], mbSize, NULL, nullptr);
	return u8str;
}