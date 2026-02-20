if(WIN32)
    add_executable(HWMix WIN32
        main_windows.cpp
        ${CMAKE_SOURCE_DIR}/resources.rc
    )
else()
    add_executable(HWMix main_linux.cpp)
endif()
