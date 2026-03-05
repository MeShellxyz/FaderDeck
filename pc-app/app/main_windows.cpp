#include "AppHost/AppHost.h"
#include "Config/AppConfig.h"
#include "Config/ConfigStore.h"
#include "WinUI/WinUI.h"

#include <iostream>
#include <string>
#include <vector>


#include <atomic>
#include <thread>
#include <windows.h>


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {

    ConfigStore configStore;
    AppConfig appConfig;
    if (!configStore.loadConfig(appConfig)) {
        return 1;
    }

    try {
        AppHost appHost(appConfig);
        appHost.run();
        
        WinUI::runMessageLoop(hInstance, nCmdShow);

        appHost.stop();
    } catch (const std::exception &e) {
        return 1;
    }

    return 0;
}