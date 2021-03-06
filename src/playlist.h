#pragma once

#include <Windows.h>
#define NOMINMAX
#include <string>
#include <forward_list>
#include <vector>
#include <filesystem>
#include "ImageFormats.h"
#include "error.h"

// For natural sort function StrCmpLogicalW
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

enum class PlaylistSortMethod {
	ByName,
	ByDateModified
};

enum class PlaylistPos {
	Start,
	Current,
	End
};

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
		std::filesystem::path path;

	private:
		PlaylistSortMethod sortMethod = PlaylistSortMethod::ByName;

	public:
		Playlist();
		Playlist(const std::wstring& initialFile, PlaylistSortMethod sortMethod = PlaylistSortMethod::ByName);
		void Refresh();
		int GeneratePlaylist(const std::wstring& initialFile);
		void Clear();
		void Sort();
		int OpenPrevDir();
		int OpenNextDir();
		void SetSortingMethod(PlaylistSortMethod sortMethod);
		int MoveCursor(std::wstring filename);
		bool Move(PlaylistPos pos, int mod);
		inline PlaylistEntry* Current();
		void EraseCurrent();
		PlaylistEntry* Next();
		PlaylistEntry* Prev();
		int Add(std::wstring filename, ImageFormat format, long long unixLastWrite);
};
