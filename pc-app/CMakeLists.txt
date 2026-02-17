cmake_minimum_required(VERSION 3.5)
project(VolWare VERSION 0.1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Define Windows API target version to avoid the warning
if(MSVC)
    add_definitions(-D_WIN32_WINNT=0x0601) 
    add_compile_options(/utf-8)
endif()

# Add compiler definitions for OS
if (WIN32)
    add_compile_definitions(WINDOWS)
    set(PLATFORM_SOURCES
        src/VolumeController/VolumeControllerImpl_Windows.cpp
        src/WindowsTray.cpp)
elseif (UNIX)
    add_compile_definitions(LINUX)
    set(PLATFORM_SOURCES
        src/VolumeController/VolumeControllerImpl_Linux.cpp)
elseif (APPLE)
    add_compile_definitions(MACOS)
    set(PLATFORM_SOURCES
        src/VolumeController/VolumeControllerImpl_MacOS.cpp)
else()
    message(FATAL_ERROR "Unsupported OS")
endif()

find_package(yaml-cpp CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS system asio)

set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Don't build yaml-cpp tools" FORCE)

# Create component libraries
# 1. Configuration Library
add_library(Configuration STATIC
    src/Config.cpp
)
target_include_directories(Configuration PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(Configuration PRIVATE yaml-cpp::yaml-cpp)

# 2. Serial Communication Library
add_library(SerialComm STATIC
    src/SerialReader.cpp
)
target_include_directories(SerialComm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(SerialComm PRIVATE boost::boost)

# 3. Volume Control Library (platform-independent part)
add_library(VolumeControl STATIC
    src/VolumeController/VolumeController.cpp
)
target_include_directories(VolumeControl PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/VolumeController
)

# 4. Platform-specific Library
add_library(PlatformSpecific STATIC
    ${PLATFORM_SOURCES}
)
target_include_directories(PlatformSpecific PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/VolumeController
)
target_link_libraries(PlatformSpecific PUBLIC VolumeControl)

# Create the executable
add_executable(${PROJECT_NAME} src/main.cpp)

# Windows-specific settings for the main executable only
if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )

    set(RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources.rc)
    target_sources(${PROJECT_NAME} PRIVATE ${RESOURCE_FILES})
endif()

# Include directories for main executable
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/VolumeController
)

# Link the component libraries to the main executable
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Configuration
    SerialComm
    VolumeControl
    PlatformSpecific
    yaml-cpp::yaml-cpp
    boost::boost
)