add_library(AudioDriver STATIC)

target_include_directories(AudioDriver PUBLIC include)
target_include_directories(AudioDriver PRIVATE src)

if(WIN32)
    target_sources(AudioDriver PRIVATE src/AudioDriver_Win.cpp)
else()
    target_sources(AudioDriver PRIVATE src/AudioDriver_Linux.cpp)
endif()