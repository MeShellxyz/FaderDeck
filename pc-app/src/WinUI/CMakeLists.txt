add_library(WinUI STATIC "src/WinUI.cpp" "resources.rc")

target_include_directories(WinUI PUBLIC include)
target_include_directories(WinUI PRIVATE src)