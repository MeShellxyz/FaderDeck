#include "WinUI/WinUI.h"

#include "WinUI/resources.h"

#include <shellapi.h>

#define WM_TRAYICON (WM_APP + 1)


WinUI* WinUI::s_instance = nullptr;

WinUI::WinUI(HINSTANCE hInstance) : m_hInstance(hInstance), m_hWnd(nullptr) {

    // Register window class
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = m_hInstance;
    wc.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_ICON));
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = MAKEINTRESOURCEW(IDR_MENU);
    wc.lpszClassName = WINDOW_CLASS_NAME;

    RegisterClassExW(&wc);

    WCHAR title[100];
    LoadStringW(m_hInstance, IDS_APP_TITLE, title, ARRAYSIZE(title));
    m_hWnd = CreateWindowExW(0, WINDOW_CLASS_NAME, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                            NULL, NULL, m_hInstance, NULL);

    if(!m_hWnd) {
        MessageBoxW(nullptr, L"Failed to create main window.", L"Error", MB_ICONERROR);
        PostQuitMessage(1);
        return;
    }
    s_instance = this;
}

WinUI::~WinUI() {
    if (m_hWnd) {
        s_instance->deleteNotifyIcon(m_hWnd);
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
        s_instance = nullptr;
    }
}

void WinUI::runMessageLoop() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
}

LRESULT CALLBACK WinUI::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            if (!s_instance->addNotifyIcon(hwnd)) {
                MessageBoxW(hwnd, L"Failed to add notification icon.", L"Error", MB_ICONERROR);
                PostQuitMessage(1);
                return -1;
            }
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDM_CONFIG:
                    // Handle config menu item
                    break;
                case IDM_EXIT:
                    DestroyWindow(hwnd);
                    break;
            }
            break;

        case WM_TRAYICON:
            switch (LOWORD(lParam))
            {
            case WM_CONTEXTMENU:
                POINT const pt = { LOWORD(wParam), HIWORD(wParam) };
                s_instance->showContextMenu(pt);
                break;
            }
        case WM_DESTROY:
            s_instance->deleteNotifyIcon(hwnd);
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

BOOL WinUI::addNotifyIcon(HWND hwnd) {
    NOTIFYICONDATAW nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIconW(m_hInstance, MAKEINTRESOURCEW(IDI_ICON));
    LoadStringW(m_hInstance, IDS_APP_TITLE, nid.szTip, ARRAYSIZE(nid.szTip));
    Shell_NotifyIconW(NIM_ADD, &nid);

    nid.uVersion = NOTIFYICON_VERSION_4;
    return Shell_NotifyIconW(NIM_SETVERSION, &nid);
}

BOOL WinUI::deleteNotifyIcon(HWND hwnd) {
    NOTIFYICONDATAW nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hwnd;
    nid.uID = 1;
    return Shell_NotifyIconW(NIM_DELETE, &nid);
}

void WinUI::showContextMenu(POINT pt) {
    HMENU hMenu = LoadMenu(m_hInstance, MAKEINTRESOURCE(IDR_MENU));
    if (hMenu) {
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if (hSubMenu) {
            SetForegroundWindow(m_hWnd);

            UINT uFlags = TPM_RIGHTBUTTON;
            if(GetSystemMetrics(SM_MENUDROPALIGNMENT) == 0) {
                uFlags |= TPM_LEFTALIGN;
            } else {
                uFlags |= TPM_RIGHTALIGN;
            }

            TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, m_hWnd, nullptr);
        }
        DestroyMenu(hMenu);
    }
}