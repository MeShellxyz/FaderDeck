add_library(Config STATIC src/ConfigStore.cpp)

target_include_directories(Config PUBLIC include)
target_include_directories(Config PRIVATE src)