#include "playlist.h"

#include <TCHAR.h>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

namespace fs = std::filesystem;

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
	{ L".jpeg", ImageFormat::JPEG },
	{ L".png", ImageFormat::PNG },
	{ L".tga", ImageFormat::TGA },
	{ L".bmp", ImageFormat::BMP },
	{ L".gif", ImageFormat::GIF },
	{ L".webp", ImageFormat::WEBP },
	//{ L".psd", ImageFormat::PSD },
	//{ L".dds", ImageFormat::DDS },
	//{ L".tiff", ImageFormat::TIFF }
};

Playlist::Playlist() {

}

Playlist::Playlist(const std::wstring& initialFile, PlaylistSortMethod sortMethod) {
	sortMethod = sortMethod;
	GeneratePlaylist(initialFile);
}

int Playlist::Add(std::wstring full_path, ImageFormat format, long long unixLastWrite) {
	PlaylistEntry file;
	std::filesystem::path path(full_path);

	file.path = full_path;
	file.format = format;
	file.filename = path.filename().wstring();
	file.tmFileWrite = unixLastWrite;
	list.push_back(file);
	return 1;
}

void Playlist::Refresh() {
	auto cur = Current();
	std::wstring currentPath(cur->path);
	list.clear();
	GeneratePlaylist(currentPath);
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

inline PlaylistEntry* Playlist::Current() {
	if (list.size() == 0) return nullptr;
	auto it = list.begin() + offset;
	return &*it;
}

void Playlist::Clear(){
	list.clear();
}

void Playlist::EraseCurrent() {
	if (list.size() > 1) {
		list.erase(list.begin() + offset);
	}
}

bool Playlist::Move(PlaylistPos pos, int mod) {
	int offsetBefore = offset;

	int offset_base = offset;
	if (pos == PlaylistPos::Start) {
		offset_base = 0;
	}
	else if (pos == PlaylistPos::End) {
		offset_base = list.end() - list.begin();
	}

	int maxOffset = list.size() - 1; // important that it's int
	offset = offset_base + mod;
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


int Playlist::GeneratePlaylist(const std::wstring& initialPathStr) {
	if (list.size() > 0)
		list.clear();

	fs::path initialPath(initialPathStr);
	bool initialFileSpecified = !fs::is_directory(initialPath);

	this->path = initialFileSpecified ? initialPath.parent_path() : initialPath;
	std::wstring initialFileDir = this->path.wstring() + L"\\";
	

	WCHAR oldWD[1000];
	GetCurrentDirectoryW(1000, oldWD);

	if (!SetCurrentDirectoryW(initialFileDir.c_str())) {
		// TODO: Can't Open
		D4See::SetError(D4See::Error::DirectoryNotExists);
		MessageBoxW(NULL, (L"Couldn't open directory: " + initialFileDir).c_str(), L"Error", MB_ICONERROR | MB_OK);
		return NULL;
	}


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
					Add(std::wstring(initialFileDir + fd.cFileName), search->second, unixLastWrite); // name, format
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

	if (filesAdded == 0) {
		// TODO: No supported images inside a dir
		D4See::SetError(D4See::Error::EmptyPlaylist);
		MessageBoxW(NULL, (L"No supoprted files in: " + initialFileDir).c_str(), L"Error", MB_ICONERROR | MB_OK);
		return NULL;
	}

	this->Sort();

	if (initialFileSpecified)
		MoveCursor(initialPathStr);
	else
		this->offset = 0;
	//else

	return filesAdded;
}

fs::path FindSiblingDirectory(fs::path& currentDirPath, int mod){
	if (!currentDirPath.has_parent_path())
		return currentDirPath;

	fs::path rootDirPath = currentDirPath.parent_path();

	std::vector<fs::directory_entry> filelist;
	for (auto& p : fs::directory_iterator(rootDirPath)) {
		if (p.is_directory())
			filelist.push_back(p);
	}

	auto result = std::find_if(filelist.begin(), filelist.end(), [&currentDirPath](fs::directory_entry entry) {
		return (entry.path() == currentDirPath);
	});

	if (result != filelist.end()) {
		int max_offset = filelist.size() - 1;
		int offset = result - filelist.begin();
		int new_offset = offset + mod;
		new_offset = (std::max)(new_offset, 0);
		new_offset = (std::min)(new_offset, max_offset);
		auto newDirPath = filelist[new_offset].path();
		return newDirPath;
	}
	else {
		return currentDirPath;
	}
}

int Playlist::OpenPrevDir() {
	auto siblingDir = FindSiblingDirectory(this->path, -1);
	// If can't switch for whatever reason FindSiblingDirectory returns current path
	if (siblingDir != this->path) {
		list.clear();
		GeneratePlaylist(siblingDir.wstring());
		return 1;
	}
	return 0;
}
int Playlist::OpenNextDir() {
	auto siblingDir = FindSiblingDirectory(this->path, +1);
	// If can't switch for whatever reason FindSiblingDirectory returns current path
	if (siblingDir != this->path) { 
		list.clear();
		GeneratePlaylist(siblingDir.wstring());
		return 1;
	}
	return 0;
}

void Playlist::Sort(){
	auto naturalSort = [](PlaylistEntry& a, PlaylistEntry& b) {
		return StrCmpLogicalW(a.filename.c_str(), b.filename.c_str()) < 0;
	};

	if (sortMethod == PlaylistSortMethod::ByDateModified) {
		auto lastModifiedSort = [&naturalSort](PlaylistEntry& a, PlaylistEntry& b) {
			return (a.tmFileWrite == b.tmFileWrite) ? naturalSort(a, b) : (a.tmFileWrite < b.tmFileWrite);
		};
		std::sort(list.begin(), list.end(), lastModifiedSort);
	}
	else {
		std::sort(list.begin(), list.end(), naturalSort);
	}
}


void Playlist::SetSortingMethod(PlaylistSortMethod newSortMethod) {
	sortMethod = newSortMethod;

	if (list.size() > 0) {
		auto cur = Current();
		std::wstring oldCursor = cur->path;

		this->Sort();
		
		MoveCursor(oldCursor);
	}
}