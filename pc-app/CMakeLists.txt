cmake_minimum_required(VERSION 3.5...4.2)
project(VolWare VERSION 0.1.0)

include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT lto_error)

if(lto_supported)
    message(STATUS "LTO/IPO supported and enabled.")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(WARNING "LTO/IPO not supported: ${lto_error}.")
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Define Windows API target version to avoid the warning
if(MSVC)
    add_definitions(-D_WIN32_WINNT=0x0601) 
    add_compile_options(/utf-8)
endif()

# set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Don't build yaml-cpp tools" FORCE)

add_subdirectory(src/Core)
add_subdirectory(src/Config)
add_subdirectory(src/AudioDriver)
add_subdirectory(src/SerialListener)
add_subdirectory(src/AppHost)

add_subdirectory(app)