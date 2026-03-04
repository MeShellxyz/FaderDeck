#pragma once

#include <windows.h>

class WinUI
{
    public:
        WinUI(HINSTANCE hInstance);
        ~WinUI();

        void runMessageLoop();

    private:
        void registerWindowClass();
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        BOOL addNotifyIcon(HWND);
        BOOL deleteNotifyIcon(HWND);
        void showContextMenu(POINT pt);

        constexpr const static wchar_t *WINDOW_CLASS_NAME = L"NotificationWindowClass";

        HINSTANCE m_hInstance;
        HWND m_hWnd;

        static WinUI* s_instance; 



};