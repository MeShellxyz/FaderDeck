add_library(SerialListener STATIC src/SerialListener.cpp)

target_include_directories(SerialListener PUBLIC include)
target_include_directories(SerialListener PRIVATE src)

find_package(Boost CONFIG REQUIRED COMPONENTS system asio)

target_link_libraries(SerialListener PRIVATE boost::boost)