add_library(AppHost STATIC src/AppHost.cpp)

target_include_directories(AppHost PUBLIC include)
target_include_directories(AppHost PRIVATE src)

target_link_libraries(AppHost PUBLIC
    Core
    Config
    AudioDriver
    SerialListener
)