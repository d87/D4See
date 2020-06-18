#pragma once

#include <Windows.h>
#include <string>
#include <forward_list>
#include <vector>
#include "ImageFormats.h"

struct PlaylistEntry {
	std::wstring path;
	std::wstring filename;
	long long tmFileWrite;
	ImageFormat format;
};

// Playlist is in wchars, but wstring gets converted to utf8 when passed to for decode

class Playlist {
	public:
		std::vector<PlaylistEntry> list;
		int offset = 0;
		std::wstring basepath;

	public:
		Playlist();
		Playlist(const std::wstring initialFile);
		int GeneratePlaylist(const std::wstring initialFile);
		int MoveCursor(std::wstring filename);
		bool Move(int mod);
		PlaylistEntry* Current();
		PlaylistEntry* Next();
		PlaylistEntry* Prev();
		int Add(std::wstring filename, ImageFormat format, long long unixLastWrite);
};
