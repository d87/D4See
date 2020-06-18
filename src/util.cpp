#include "util.h"
#pragma warning(disable: 4996)

std::string wide_to_utf8(std::wstring wstr) {
	std::string u8str;
	int mbSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.length(), &u8str[0], 0, NULL, nullptr);
	u8str.resize(mbSize);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.length(), &u8str[0], mbSize, NULL, nullptr);
	return u8str;
}

std::filesystem::path GetExecutableDir() {
    std::filesystem::path exePath;

    WCHAR path[1000];
    GetModuleFileNameW(GetModuleHandle(NULL), path, 1000);

    exePath = path;
    return exePath.parent_path();
}

std::filesystem::path GetAppDataRoaming() {
    std::filesystem::path configRoot;
    PWSTR path_tmp;

    if (S_OK != SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path_tmp)) {
        CoTaskMemFree(path_tmp); // Free memory
        return GetExecutableDir();
    }
    configRoot = path_tmp;
    CoTaskMemFree(path_tmp); // Free memory

    return configRoot;

    //std::filesystem::path appFolder("D4See");
    //std::filesystem::path file("origin");

    //std::filesystem::path fullPath = configRoot / appFolder / file;
    //std::cout << fullPath << std::endl;
}

std::vector<std::byte> load_file(std::string const& filepath)
{
    std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);

    if (!ifs)
        throw std::runtime_error(filepath + ": " + std::strerror(errno));

    auto end = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    auto size = std::size_t(end - ifs.tellg());

    if (size == 0) // avoid undefined behavior 
        return {};

    std::vector<std::byte> buffer(size);

    if (!ifs.read((char*)buffer.data(), buffer.size()))
        throw std::runtime_error(filepath + ": " + std::strerror(errno));

    //ifs.close();

    return buffer;
}

int write_file(std::string const& filepath, const char* data, unsigned int size)
{
    std::ofstream ofs(filepath, std::ios::out | std::ios::binary);

    if (!ofs)
        throw std::runtime_error(filepath + ": " + std::strerror(errno));

    ofs.write(data, size);

    std::vector<std::byte> buffer(size);

    //ofs.close();

    return 1;
}