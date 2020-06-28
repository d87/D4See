#pragma once

#include <Windows.h>
#include <shlobj.h>
#include <filesystem>
#include <string>
#include <fstream>
#include <vector>

int CutCopyFile(std::wstring path, DWORD dropEffect = DROPEFFECT_COPY);
int DeleteFileDialog(std::wstring path, bool recycle = true);
std::wstring OpenFileDialog();

std::string wide_to_utf8(std::wstring wstr);
std::filesystem::path GetExecutableDir();
std::filesystem::path GetAppDataRoaming();
std::vector<std::byte> load_file(std::string const& filepath);
int write_file(std::string const& filepath, const char* data, unsigned int size);