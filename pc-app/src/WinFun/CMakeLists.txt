add_library(WinUI STATIC "src/WinUI.cpp" "resources.rc")
add_library(WinAutoStart STATIC "src/WinAutoStart.cpp")

target_include_directories(WinUI PUBLIC include)
target_include_directories(WinUI PRIVATE src)

target_include_directories(WinAutoStart PUBLIC include)
target_include_directories(WinAutoStart PRIVATE src)