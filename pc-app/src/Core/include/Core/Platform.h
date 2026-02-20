#pragma once

#if defined(_WIN32)
    #define HWM_PLATFORM_WINDOWS
#elif defined(__linux__)
    #define HWM_PLATFORM_LINUX
#else
    #error "Unsupported OS"
#endif