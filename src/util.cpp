#include "util.h"
#pragma warning(disable: 4996)

#ifdef _WIN32

int CutCopyFile(std::wstring path, DWORD dropEffect ) { // DROPEFFECT_MOVE or DROPEFFECT_COPY
    int result = 0;

    UINT prefDropEffectFormat = RegisterClipboardFormatA(CFSTR_PREFERREDDROPEFFECT);

    HGLOBAL hGlobalDropEffect = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, sizeof(DWORD));

    DWORD* pDropEffect = (DWORD*)GlobalLock(hGlobalDropEffect);
    *pDropEffect = dropEffect;
    GlobalUnlock(hGlobalDropEffect);

    int size = sizeof(DROPFILES) + ((path.length() + 2) * sizeof(WCHAR)); // DROPFILES struct, then double null terminated wstring
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
    DROPFILES* df = (DROPFILES*)GlobalLock(hGlobal);
    ZeroMemory(df, size);
    df->pFiles = sizeof(DROPFILES);
    df->fWide = TRUE;
    LPWSTR ptr = (LPWSTR)(df + 1); // Skip header and point to string
    lstrcpyW(ptr, path.c_str());
    GlobalUnlock(hGlobal);

    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        // A window can place more than one object on the clipboard, each representing the same information in a different clipboard format.
        // And the drop effect object kind of interacts with the HDROP object, telling the target whether to move or copy
        SetClipboardData(CF_HDROP, hGlobal);
        SetClipboardData(prefDropEffectFormat, hGlobalDropEffect);
        CloseClipboard();

        if (dropEffect == DROPEFFECT_MOVE) result = 1;
    }
    else {
        MessageBoxW(NULL, L"Couldn't open clipboard", L"ERROR", MB_ICONERROR);
    }
    return result;
}


HRESULT ShellDeleteFileOperation(IShellItem* siFile, DWORD flags)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileOperation* pfo;
        hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pfo));
        if (SUCCEEDED(hr))
        {
            pfo->SetOperationFlags(flags);
            hr = pfo->DeleteItem(siFile, NULL);
            if (SUCCEEDED(hr))
            {
                hr = pfo->PerformOperations();
            }
            pfo->Release();
        }
    }
    return hr;
}

int DeleteFileDialog(std::wstring path, bool recycle) {
    IShellItem * item = nullptr;
    SHCreateItemFromParsingName(path.c_str(), NULL, IID_PPV_ARGS(&item));

    DWORD flags = FOF_WANTNUKEWARNING;
    if (recycle) {
        flags |= FOFX_RECYCLEONDELETE;
    }

    if (SUCCEEDED(ShellDeleteFileOperation(item, flags))) {
        return 1;
    }
    return 0;
}

std::wstring OpenFileDialog() {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
        COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileOpenDialog* pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
            IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

        if (SUCCEEDED(hr))
        {
            // Show the Open dialog box.
            hr = pFileOpen->Show(NULL);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr))
            {
                IShellItem* pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // Display the file name to the user.
                    if (SUCCEEDED(hr))
                    {
                        std::wstring filepath(pszFilePath);
                        //MessageBoxW(NULL, pszFilePath, L"File Path", MB_OK);
                        CoTaskMemFree(pszFilePath);
                        return filepath;
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }
    return L"";
}

#endif


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