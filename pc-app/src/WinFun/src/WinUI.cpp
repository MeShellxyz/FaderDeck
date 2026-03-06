#include "WinUI/WinUI.h"

#include "resources.h"

#include <shellapi.h>
#include <shlobj.h>
#include <KnownFolders.h>

#include <filesystem>

#define WM_TRAYICON (WM_APP + 1)

namespace WinUI {
    struct params {
        HINSTANCE hInstance;
        UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;
    };

} // namespace WinUI

void registerWindowClass(HINSTANCE hInstance, WNDPROC lpfnWndProc);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void showContextMenu(HWND hwnd, POINT pt);
BOOL addNotifyIcon(HWND hwnd);
BOOL deleteNotifyIcon(HWND hwnd);
void openConfigInExplorer();

void WinUI::runMessageLoop(HINSTANCE hInstance, int nCmdShow) {

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to initialize COM library.", L"Error", MB_ICONERROR);
        return;
    }

    registerWindowClass(hInstance, WindowProc);

    WCHAR title[100];
    LoadStringW(hInstance, IDS_APP_TITLE, title, ARRAYSIZE(title));
    HWND hWnd = CreateWindowExW(0, L"WinUIClass", title, NULL,
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, HWND_MESSAGE, NULL, hInstance, NULL);

    if (hWnd) {
        ShowWindow(hWnd, nCmdShow);

        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    CoUninitialize();
}


void registerWindowClass(HINSTANCE hInstance, WNDPROC lpfnWndProc) {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = lpfnWndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = MAKEINTRESOURCEW(IDR_MENU);
    wc.lpszClassName = L"WinUIClass";

    RegisterClassExW(&wc);
}

void showContextMenu(HWND hwnd, POINT pt) {
    HMENU hMenu = LoadMenu(GetModuleHandleW(nullptr), MAKEINTRESOURCE(IDR_MENU));
    if (hMenu) {
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if (hSubMenu) {
            SetForegroundWindow(hwnd);

            UINT uFlags = TPM_RIGHTBUTTON;
            if (GetSystemMetrics(SM_MENUDROPALIGNMENT) == 0) {
                uFlags |= TPM_LEFTALIGN;
            } else {
                uFlags |= TPM_RIGHTALIGN;
            }
            TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, nullptr);
        }
        DestroyMenu(hMenu);
    }
}

BOOL addNotifyIcon(HWND hwnd) {
    NOTIFYICONDATAW nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_ICON));
    LoadStringW(GetModuleHandleW(nullptr), IDS_APP_TITLE, nid.szTip,
                ARRAYSIZE(nid.szTip));
    Shell_NotifyIconW(NIM_ADD, &nid);

    nid.uVersion = NOTIFYICON_VERSION_4;
    return Shell_NotifyIconW(NIM_SETVERSION, &nid);
}

BOOL deleteNotifyIcon(HWND hwnd) {
    NOTIFYICONDATAW nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hwnd;
    nid.uID = 1;
    return Shell_NotifyIconW(NIM_DELETE, &nid);
}

void openConfigInExplorer() {
    PWSTR pszPath = nullptr;

    HRESULT hr =
        SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &pszPath);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to get AppData path.", L"Error", MB_ICONERROR);
        return;
    }

    std::filesystem::path configFilePath(pszPath);
    CoTaskMemFree(pszPath);

    configFilePath /= "com.ms.volware";
    configFilePath /= "config.toml";

    ShellExecuteW(nullptr, L"open", configFilePath.c_str(), nullptr, nullptr, SW_SHOW);

}

LRESULT CALLBACK WinUI::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        if (!addNotifyIcon(hwnd)) {
            MessageBoxW(hwnd, L"Failed to add notification icon.", L"Error",
                        MB_ICONERROR);
            PostQuitMessage(1);
            return -1;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_CONFIG:
            openConfigInExplorer();
            break;
        case IDM_EXIT:
            DestroyWindow(hwnd);
            break;
        }
        break;

    case WM_TRAYICON:
        switch (LOWORD(lParam)) {
        case WM_CONTEXTMENU:
            POINT const pt = {LOWORD(wParam), HIWORD(wParam)};
            showContextMenu(hwnd, pt);
            break;
        }
        break;
    case WM_DESTROY:
        deleteNotifyIcon(hwnd);
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}