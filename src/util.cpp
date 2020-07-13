#include "util.h"
#include "D4See.h"
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

    if (item) {
        DWORD flags = FOF_WANTNUKEWARNING | FOF_FILESONLY; // | FOF_NORECURSION;
        if (recycle) {
            flags |= FOFX_RECYCLEONDELETE;
        }

        if (SUCCEEDED(ShellDeleteFileOperation(item, flags))) {
            return 1;
        }
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

std::filesystem::path GetExecutableFilePath() {
    std::filesystem::path exePath;

    WCHAR path[1000];
    GetModuleFileNameW(GetModuleHandle(NULL), path, 1000);

    exePath = path;
    return exePath;
}

std::filesystem::path GetExecutableDir() {
    return GetExecutableFilePath().parent_path();
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

std::string GetStringRegistryValue(HKEY hKey, std::string valueName) {
    DWORD dwType;
    DWORD dataSize = 200;
    DWORD err = RegQueryValueExA(hKey, valueName.c_str(), 0, &dwType, NULL, &dataSize);
    if (err != ERROR_SUCCESS) {
        MessageBoxW(NULL, L"Couldn't query Registry key", L"Registry Error", MB_ICONERROR);
        return nullptr;
    }

    std::string data(dataSize-1, 0); // Excluding trailing 0
    RegQueryValueExA(hKey, valueName.c_str(), 0, &dwType, (BYTE*)&data[0], &dataSize);
    return data;
}

int KeyExists(HKEY hKey, std::string subKey) {
    HKEY hk;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, subKey.c_str(), 0, KEY_QUERY_VALUE, &hk) != ERROR_SUCCESS) {
        return 0;
    }
    RegCloseKey(hk);
    return 1;
}

int IsExtensionAssociated(std::string ext, std::string progClass) {
    if (!KeyExists(HKEY_CURRENT_USER, "Software\\Classes\\" + progClass))
        return 0;

    HKEY hk;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, ("Software\\Classes\\" + ext).c_str(), 0, KEY_QUERY_VALUE, &hk) != ERROR_SUCCESS) {
        return 0;
    }

    std::string data = GetStringRegistryValue(hk, "");

    RegCloseKey(hk);

    return (data == progClass);
}

void AddFileHandlerClass(std::string progClass, std::wstring iconPathStr) {
    HKEY hKey;
    DWORD disposition; // Receives REG_OPENED_EXISTING_KEY or REG_CREATED_NEW_KEY
    if (RegCreateKeyEx(HKEY_CURRENT_USER, ("Software\\Classes\\" + progClass).c_str(), 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &disposition) == ERROR_SUCCESS) {

        HKEY iconKey;
        if (RegCreateKeyEx(hKey, "DefaultIcon", 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &iconKey, &disposition) == ERROR_SUCCESS) {

            auto exeDir = GetExecutableDir();
            std::filesystem::path iconPath(iconPathStr);
            auto fullIconPath = exeDir / iconPath;
            std::wstring quotesPathString = L'"'+fullIconPath.wstring()+L'"';
            RegSetValueExW(iconKey, L"", 0, REG_SZ, (BYTE*)quotesPathString.c_str(), quotesPathString.length() * sizeof(WCHAR));
            RegCloseKey(iconKey);
        }

        HKEY shellOpenCommandKey;
        if (RegCreateKeyEx(hKey, "shell\\open\\command", 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &shellOpenCommandKey, &disposition) == ERROR_SUCCESS) {

            auto exePath = GetExecutableFilePath();
            std::wstring quotesPathString = L'"' + exePath.wstring() + L"\" \"%1\"" ;
            RegSetValueExW(shellOpenCommandKey, L"", 0, REG_SZ, (BYTE*)quotesPathString.c_str(), quotesPathString.length() * sizeof(WCHAR));
            RegCloseKey(shellOpenCommandKey);
        }

        HKEY shellOpenKey;
        if (RegCreateKeyEx(hKey, "shell\\open", 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &shellOpenKey, &disposition) == ERROR_SUCCESS) {

            std::wstring friendlyName = L"D4See";
            RegSetValueExW(shellOpenKey, L"FriendlyAppName", 0, REG_SZ, (BYTE*)friendlyName.c_str(), friendlyName.length() * sizeof(WCHAR));
            RegCloseKey(shellOpenKey);
        }

        RegCloseKey(hKey);
    }
}

void RemoveFileHandlerClass(std::string progClass) {
    RegDeleteKeyExA(HKEY_CURRENT_USER, ("Software\\Classes\\" + progClass).c_str(), KEY_WRITE, 0);
}

void AssignExtensionToFileTypeHandler(std::string ext, std::string progClass) {
    HKEY hKey;
    DWORD disposition; // Receives REG_OPENED_EXISTING_KEY or REG_CREATED_NEW_KEY
    if (RegCreateKeyEx(HKEY_CURRENT_USER, ("Software\\Classes\\" + ext).c_str(), 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &disposition) == ERROR_SUCCESS) {

        RegSetValueExA(hKey, "", 0, REG_SZ, (BYTE*)progClass.c_str(), progClass.length() * sizeof(char) );

        HKEY OpenWithProgids;
        if (RegCreateKeyEx(hKey, "OpenWithProgids", 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &OpenWithProgids, &disposition) == ERROR_SUCCESS) {
            RegSetValueExA(OpenWithProgids, progClass.c_str(), 0, REG_SZ, (BYTE*)"", 0);
            RegCloseKey(OpenWithProgids);
        }

        RegCloseKey(hKey);
    }
}

void RemoveFileHandlerFromExtension(std::string ext, std::string progClass) {
    HKEY hk;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, ("Software\\Classes\\" + ext).c_str(), 0, KEY_ALL_ACCESS, &hk) != ERROR_SUCCESS) {
        return;
    }
    std::string currentClass = GetStringRegistryValue(hk, "");
    if (currentClass == progClass) {
        RegSetValueExA(hk, "", 0, REG_SZ, NULL, 0);
    }
    HKEY OpenWithProgids;
    if (RegOpenKeyEx(hk, "OpenWithProgids", 0, KEY_WRITE, &OpenWithProgids) == ERROR_SUCCESS) {
        RegDeleteValueA(OpenWithProgids, progClass.c_str());
        RegCloseKey(OpenWithProgids);
    }
    RegCloseKey(hk);
}

void AssociateAllTypes() {
    AddFileHandlerClass("D4See.png", L"Icons\\PNG_ICON.ico"); // "H:\soft\D4See\D4See.exe" "%1"
    AssignExtensionToFileTypeHandler(".png", "D4See.png");

    AddFileHandlerClass("D4See.bmp", L"Icons\\BMP_ICON.ico");
    AssignExtensionToFileTypeHandler(".bmp", "D4See.bmp");

    AddFileHandlerClass("D4See.jpg", L"Icons\\JPG_ICON.ico");
    AssignExtensionToFileTypeHandler(".jpg", "D4See.jpg");

    AddFileHandlerClass("D4See.gif", L"Icons\\GIF_ICON.ico");
    AssignExtensionToFileTypeHandler(".gif", "D4See.gif");

    AddFileHandlerClass("D4See.tga", L"Icons\\TGA_ICON.ico");
    AssignExtensionToFileTypeHandler(".tga", "D4See.tga");

    //AddFileHandlerClass("D4See.img", L"Icons\\IMG_ICON.ico");
    //AssignExtensionToFileTypeHandler(".dds", "D4See.img");

    AddFileHandlerClass("D4See.webp", L"Icons\\WEBP_ICON.ico");
    AssignExtensionToFileTypeHandler(".webp", "D4See.webp");
}

void RemoveAssociations() {
    RemoveFileHandlerFromExtension(".png", "D4See.png");
    RemoveFileHandlerClass("D4See.png");

    RemoveFileHandlerFromExtension(".bmp", "D4See.bmp");
    RemoveFileHandlerClass("D4See.bmp");

    RemoveFileHandlerFromExtension(".jpg", "D4See.jpg");
    RemoveFileHandlerClass("D4See.jpg");

    RemoveFileHandlerFromExtension(".gif", "D4See.gif");
    RemoveFileHandlerClass("D4See.gif");

    RemoveFileHandlerFromExtension(".tga", "D4See.tga");
    RemoveFileHandlerClass("D4See.tga");

    RemoveFileHandlerFromExtension(".webp", "D4See.webp");
    RemoveFileHandlerClass("D4See.webp");
}