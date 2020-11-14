#pragma once

#include <Windows.h>
#include <shlobj.h>
#include <filesystem>
#include <string>
#include <fstream>
#include <vector>

int CutCopyFile(const std::wstring& path, DWORD dropEffect = DROPEFFECT_COPY);
int DeleteFileDialog(const std::wstring& path, bool recycle = true);
std::wstring OpenFileDialog();

std::string wide_to_utf8(const std::wstring& wstr);
std::filesystem::path GetExecutableFilePath();
std::filesystem::path GetExecutableDir();
std::filesystem::path GetAppDataRoaming();
std::vector<std::byte> load_file(std::string const& filepath);
int write_file(std::string const& filepath, const char* data, unsigned int size);
int IsExtensionAssociated(const std::string& ext, const std::string& progClass);
void AddFileHandlerClass(const std::string& name, const std::wstring& iconPath);
void AssignExtensionToFileTypeHandler(const std::string& ext, const std::string& progClass);
void AssociateAllTypes();
void RemoveAssociations();