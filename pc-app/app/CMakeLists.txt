if(WIN32)
    add_executable(${CMAKE_PROJECT_NAME} WIN32
        main_windows.cpp
        ${CMAKE_SOURCE_DIR}/src/WinFun/resources.rc
    )
else()
    add_executable(${CMAKE_PROJECT_NAME} main_linux.cpp)
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE 
    AppHost    
    Config
    WinUI
    WinAutoStart
)