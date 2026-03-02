#include "AppHost/AppHost.h"
#include "Config/AppConfig.h"
#include "Config/ConfigStore.h"

#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)

// #include "WindowsAutostart.h"
// #include "WindowsTray.h"
#include <atomic>
#include <windows.h>

// Global running flag
std::atomic<bool> g_running = true;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {

    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        AllocConsole();
    }
    FILE *fp = nullptr;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);
    std::ios::sync_with_stdio();
    std::cout << std::endl;
    // int main() {

    ConfigStore configStore;

    std::cout << "[INIT] Done" << std::endl;

    AppConfig appConfig;
    configStore.loadConfig(appConfig);

    std::cout << "[CONFIG] Loaded" << std::endl;

    std::cout << "[CONFIG] Serial Port: " << appConfig.serialConfig.com_port
              << " Baud Rate: " << appConfig.serialConfig.baud_rate
              << std::endl;
    std::cout << "[CONFIG] Audio Invert Sliders: "
              << appConfig.audioConfig.invert_sliders
              << " Mute Buttons: " << appConfig.audioConfig.mute_buttons
              << " Num Channels: " << appConfig.audioConfig.num_channels
              << std::endl;
    std::cout << "[CONFIG] Process Channel Mapping: " << std::endl;
    for (const auto &mapping : appConfig.audioConfig.processChannelMapping) {
        std::wcout << L"  Process: " << mapping.first << L" -> Channel: "
                   << mapping.second << std::endl;
    }
    std::cout << std::endl;

    fclose(stdout);
    fclose(stderr);
    fclose(stdin);
    FreeConsole();
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