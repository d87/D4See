#include "playlist.h"

#include <windows.h>
#include <TCHAR.h>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

long long WindowsTickToUnixSeconds(FILETIME ft)
{
	const long long UNIX_TIME_START = 0x019DB1DED53E8000; //January 1, 1970 (start of Unix epoch) in "ticks"
	const long long TICKS_PER_SECOND = 10000000; //a tick is 100ns

	LARGE_INTEGER li;
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	return (li.QuadPart - UNIX_TIME_START) / TICKS_PER_SECOND;
}

const std::unordered_map<std::wstring, ImageFormat> exts = {
	{ L".jpg", ImageFormat::JPEG },
	{ L".png", ImageFormat::PNG },
	{ L".tga", ImageFormat::TGA },
	{ L".bmp", ImageFormat::BMP },
	{ L".gif", ImageFormat::GIF },
	// OIIO supports normal webp, but animated ones fail to load at all because "Unsupported feature"
	// Should re-eanble webp after image load error handling is done
	//{ L".webp", ImageFormat::WEBP },
	{ L".tiff", ImageFormat::TIFF }
};

Playlist::Playlist() {

}

Playlist::Playlist(std::wstring initialFile) {
	GeneratePlaylist(initialFile);
}

int Playlist::Add(std::wstring full_path, ImageFormat format, long long unixLastWrite) {
	PlaylistEntry file;
	std::filesystem::path path(full_path);

	file.path = full_path;
	file.format = format;
	file.filename = path.filename().wstring();
	file.tFileWrite = unixLastWrite;
	list.push_back(file);
	return 1;
}

int Playlist::MoveCursor(std::wstring filename) {
	for ( auto it = list.begin(); it != list.end(); ++it) {
		if (it->path == filename) {
			offset = it - list.begin();
			return offset;
		}
	}
	return NULL;
}

PlaylistEntry* Playlist::Current() {
	auto it = list.begin() + offset;
	return &*it;
}

bool Playlist::Move(int mod) {
	int offsetBefore = offset;
	int maxOffset = list.size() - 1; // important that it's int
	offset += mod;
	if (offset > maxOffset) {
		offset = maxOffset;
	}
	if (offset < 0) {
		offset = 0;
	}
	return offset != offsetBefore;
}

PlaylistEntry* Playlist::Next() {
	if ((offset + 1) <= list.size() - 1) {
		auto it = list.begin() + offset + 1;
		return &*it;
	}
	return nullptr;
}

PlaylistEntry* Playlist::Prev() {
	if ((offset - 1) >= 0) {
		auto it = list.begin() + offset - 1;
		return &*it;
	}
	return nullptr;
}


int Playlist::GeneratePlaylist(std::wstring initialFile) {
	auto found = initialFile.rfind(L"\\");
	if (found == std::string::npos)
		// No directory in initial filename, should do cwd
		return NULL;

	std::wstring initialFileDir = initialFile.substr(0, found+1);

	basepath = initialFileDir;

	WCHAR oldWD[1000];
	GetCurrentDirectoryW(1000, oldWD);

	SetCurrentDirectoryW(initialFileDir.c_str());

	int filesAdded = 0;
	HANDLE hSearch;
	WIN32_FIND_DATAW fd;
	bool fFinished = false;
	hSearch = FindFirstFileW(L"*.*", &fd);
	if (hSearch == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}
	while (!fFinished)
	{
		bool isDirectory = fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
		if (!isDirectory) {
			std::wstring cFilename = fd.cFileName;
			auto found = cFilename.rfind(L".");
			if (found != std::string::npos) {

				std::wstring ext = cFilename.substr(found);
				std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);


				auto search = exts.find(ext);
				if (search != exts.end()) {

					//SYSTEMTIME stLastWriteTime;
					long long unixLastWrite = WindowsTickToUnixSeconds(fd.ftLastWriteTime);
					//FileTimeToSystemTime(&fd.ftLastWriteTime, &stLastWriteTime);
					Add(std::wstring(basepath+fd.cFileName), search->second, unixLastWrite); // name, format
					filesAdded++;
				}
			}
		}

		if (!FindNextFileW(hSearch, &fd))
		{
			int error = GetLastError();
			if (GetLastError() == ERROR_NO_MORE_FILES)
			{
				//No more files. Search completed
				fFinished = TRUE;
			}
			else
			{
				//Couldn't find next file
				fFinished = TRUE;
				//return filesAdded;
			}
		}
	}

	SetCurrentDirectoryW(oldWD);

	MoveCursor(initialFile);

	return filesAdded;
}
