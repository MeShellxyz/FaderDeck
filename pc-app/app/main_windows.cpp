#include "AppHost/AppHost.h"
#include "Config/AppConfig.h"
#include "Config/ConfigStore.h"
#include "WinUI/WinUI.h"

#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)

// #include "WindowsAutostart.h"
// #include "WindowsTray.h"
#include <atomic>
#include <thread>
#include <windows.h>

// Global running flag
std::atomic<bool> g_isRunning{true};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {

    // if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
    //     AllocConsole();
    // }
    // FILE *fp = nullptr;
    // freopen_s(&fp, "CONOUT$", "w", stdout);
    // freopen_s(&fp, "CONOUT$", "w", stderr);
    // freopen_s(&fp, "CONIN$", "r", stdin);
    // std::ios::sync_with_stdio();
    // std::cout << std::endl;
    // int main() {

    ConfigStore configStore;
    AppConfig appConfig;
    if (!configStore.loadConfig(appConfig)) {
        // std::cerr << "Failed to load config. Using defaults." << std::endl;
    }
    // std::cout << "[INIT] Done" << std::endl;

    try {
        AppHost appHost(appConfig, g_isRunning);
        // std::jthread hostThread([&appHost] { appHost.run(); });
        appHost.run();
        
        std::this_thread::sleep_for(std::chrono::minutes(2));
        // WinUI::runMessageLoop(hInstance, nCmdShow);
        // g_isRunning.store(false, std::memory_order_release);
    } catch (const std::exception &e) {
        // std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << std::endl;

    // fclose(stdout);
    // fclose(stderr);
    // fclose(stdin);
    // FreeConsole();
    return 0;
    // try {
    //     // Load configuration

    //     // Create system tray icon
    //     WindowsTray tray(hInstance, "VolWare Volume Controller");

    //     // Set exit callback to cleanup gracefully
    //     tray.setOnExitCallback([&]() {
    //         g_running = false;
    //         PostQuitMessage(0);
    //     });

    //     // Main message loop
    //     MSG msg;
    //     while (g_running) {
    //         if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    //             TranslateMessage(&msg);
    //             DispatchMessage(&msg);
    //             if (msg.message == WM_QUIT) {
    //                 g_running = false;
    //             }
    //         } else {
    //             // Sleep to reduce CPU usage in the main thread
    //             Sleep(100);
    //         }
    //     }

    //     return 0;
    // } catch (const std::exception &e) {
    //     std::cerr << "Error: " << e.what() << std::endl;
    //     return 1;
    // }
}

#endif // _WIN32 || _WIN64