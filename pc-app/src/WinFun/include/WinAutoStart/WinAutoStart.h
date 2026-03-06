#pragma once

#include <string> 

namespace WinAutoStart {
    bool isAutoStartEnabled(const std::wstring &appName);
    void setAutoStart(const std::wstring &appName);
    void resetAutoStart(const std::wstring &appName);
    void handleAutoStart(const std::wstring &appName, bool enable);
}