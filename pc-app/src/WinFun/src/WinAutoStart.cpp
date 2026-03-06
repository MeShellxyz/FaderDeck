#include "WinAutoStart/WinAutoStart.h"

#include <filesystem>
#include <shlobj.h>
#include <windows.h>

constexpr LPCWSTR subKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

inline std::filesystem::path getExePath() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    return std::filesystem::path(exePath);
}

bool WinAutoStart::isAutoStartEnabled(const std::wstring &appName) {
    wchar_t value[MAX_PATH];
    DWORD valueSize = sizeof(value);

    LSTATUS status = RegGetValueW(HKEY_CURRENT_USER, subKey, appName.c_str(),
                                  RRF_RT_REG_SZ, nullptr, value, &valueSize);

    if (status == ERROR_SUCCESS) {
        std::filesystem::path regPath(value);
        std::filesystem::path exePath = getExePath();
        return regPath == exePath;
    }

    return false;
}

void WinAutoStart::setAutoStart(const std::wstring &appName) {
    std::filesystem::path exePath = getExePath();
    RegSetKeyValueW(HKEY_CURRENT_USER, subKey, appName.c_str(), REG_SZ,
                    exePath.c_str(),
                    (exePath.wstring().size() + 1) * sizeof(wchar_t));
}

void WinAutoStart::resetAutoStart(const std::wstring &appName) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, subKey, 0, KEY_SET_VALUE, &hKey) ==
        ERROR_SUCCESS) {
        RegDeleteValueW(hKey, appName.c_str());
        RegCloseKey(hKey);
    }
}

void WinAutoStart::handleAutoStart(const std::wstring &appName, bool enable) {
    if (isAutoStartEnabled(appName) == enable) return;
    enable ? setAutoStart(appName) : resetAutoStart(appName);
}