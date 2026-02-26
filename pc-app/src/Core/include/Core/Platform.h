#pragma once

#if defined(_WIN32)
    #define VW_PLATFORM_WINDOWS 1
#elif defined(__linux__)
    #define VW_PLATFORM_LINUX 1
#else
    #error "Unsupported OS"
#endif
