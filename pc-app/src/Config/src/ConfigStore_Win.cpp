#include "Config/ConfigStore.h"

#include <filesystem>
#include <windows.h>
#include <shlobj.h>
#include <KnownFolders.h>

ConfigStore::ConfigStore() {
    // Get the path to the user's AppData\Roaming directory
    PWSTR pszPath = nullptr;
    HRESULT hr =
        SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &pszPath);

    if (FAILED(hr)) throw std::runtime_error("Failed to get AppData path");

    std::filesystem::path appDataPath(pszPath);
    CoTaskMemFree(pszPath);
    m_configPath = appDataPath / "com.ms.faderdeck" / "config.toml";

    // ensure file exists if not create
    if (!std::filesystem::exists(m_configPath)) {
        std::filesystem::create_directories(m_configPath.parent_path());
        saveDefaultConfig();
    }
}