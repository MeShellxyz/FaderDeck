#pragma once

#include <windows.h>

namespace WinUI {
    // void initialize(HINSTANCE hInstance);
    void runMessageLoop(HINSTANCE hInstance, int nCmdShow);

    LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
}