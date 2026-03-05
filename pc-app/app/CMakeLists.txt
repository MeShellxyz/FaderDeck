if(WIN32)
    add_executable(HWMix WIN32
        main_windows.cpp
        ${CMAKE_SOURCE_DIR}/src/WinUI/resources.rc
    )
else()
    add_executable(HWMix main_linux.cpp)
endif()

target_link_libraries(HWMix PRIVATE 
    AppHost    
    Config
    WinUI
)