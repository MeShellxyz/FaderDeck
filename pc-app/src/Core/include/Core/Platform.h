#pragma once

#if defined(_WIN32)
    #define FD_PLATFORM_WINDOWS 1
#elif defined(__linux__)
    #define FD_PLATFORM_LINUX 1
#else
    #error "Unsupported OS"
#endif
