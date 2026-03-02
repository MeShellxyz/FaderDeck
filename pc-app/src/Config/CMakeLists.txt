add_library(Config STATIC src/ConfigStore.cpp)

target_include_directories(Config PUBLIC include)
target_include_directories(Config PRIVATE src)

if (WIN32)
    target_sources(Config PRIVATE src/ConfigStore_Win.cpp)
else()
    target_sources(Config PRIVATE src/ConfigStore_Linux.cpp)
endif()

find_package(tomlplusplus CONFIG REQUIRED)
target_link_libraries(Config PUBLIC tomlplusplus::tomlplusplus)