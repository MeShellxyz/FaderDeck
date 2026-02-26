add_library(AppHost STATIC src/AppHost.cpp)

target_include_directories(AppHost PUBLIC include)
target_include_directories(AppHost PRIVATE src)