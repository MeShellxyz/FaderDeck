#include "Config.h"
#include "SerialReader.h"
#include "VolumeController.h"
#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)

#include "WindowsAutostart.h"
#include "WindowsTray.h"
#include <atomic>
#include <windows.h>

// Global running flag
std::atomic<bool> g_running = true;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    try {
        // Load configuration
        Config config;

        // Set auto-start based on config
        AutoStart::SetAutoStart(config.isAutoStart());

        // Initialize volume controller
        VolumeController volumeController;
        Sleep(100);

        // Initialize serial communication
        SerialReader serialReader(config.getComPort(), config.getBaudRate());

        // Set appropriate callback based on mute button configuration
        if (config.isMuteButtons()) {
            // Handle both volume and mute data
            serialReader.setCallback([&volumeController,
                                      &config](const std::vector<int> &data) {
                for (int i = 0; i < data.size() && i < config.getChannelCount();
                     ++i) {
                    // Calculate volume level (0-1023 → 0.0-1.0)
                    float volumeLevel = static_cast<float>(data[i]) / 1024.0f;

                    // Invert if configured
                    if (config.isInvertSlider()) {
                        volumeLevel = 1.0f - volumeLevel;
                    }

                    // Get mute state
                    int mute = data[i + config.getChannelCount()];

                    // Apply to all applications mapped to this channel
                    const std::vector<std::string> &apps =
                        config.getChannelApps().at(i);
                    volumeController.setVolume(apps, volumeLevel);
                    volumeController.setMute(apps, mute);
                }
            });
        } else {
            // Handle volume data only
            serialReader.setCallback([&volumeController,
                                      &config](const std::vector<int> &data) {
                for (int i = 0; i < data.size() && i < config.getChannelCount();
                     ++i) {
                    // Calculate volume level (0-1023 → 0.0-1.0)
                    float volumeLevel = static_cast<float>(data[i]) / 1024.0f;

                    // Invert if configured
                    if (config.isInvertSlider()) {
                        volumeLevel = 1.0f - volumeLevel;
                    }

                    // Apply to all applications mapped to this channel
                    const std::vector<std::string> &apps =
                        config.getChannelApps().at(i);
                    volumeController.setVolume(apps, volumeLevel);
                }
            });
        }

        // Set sync message for serial communication
        serialReader.setSyncMessage("s");

        // Start serial communication
        if (!serialReader.start()) {
            std::cerr << "Failed to start serial reader." << std::endl;
            return 1;
        }

        // Create system tray icon
        WindowsTray tray(hInstance, "VolWare Volume Controller");

        // Set exit callback to cleanup gracefully
        tray.setOnExitCallback([&]() {
            g_running = false;
            PostQuitMessage(0);
        });

        // Main message loop
        MSG msg;
        while (g_running) {
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                if (msg.message == WM_QUIT) {
                    g_running = false;
                }
            } else {
                // Sleep to reduce CPU usage in the main thread
                Sleep(100);
            }
        }

        // Cleanup
        serialReader.stop();
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

#endif // _WIN32 || _WIN64