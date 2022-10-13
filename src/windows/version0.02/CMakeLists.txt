cmake_MINIMUM_REQUIRED(VERSION 3.11)

project(straydog VERSION 0.02)

set(CMAKE_CXX_STANDARD 11)

SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 -pthread")

find_package(Threads REQUIRED)

# add_subdirectory(test)
# find_package(OpenSSL REQUIRED)

include_directories("D:/Program Files (x86)/OpenSSL-Win64/include")
include_directories("D:/Program Files (x86)/WinDivert-2.2.1-A/include")

link_directories("D:/Program Files (x86)/OpenSSL-Win64/lib/")
link_directories("D:/Program Files (x86)/WinDivert-2.2.1-A/x64")

add_executable(straydog main.cpp straydog.cpp proxyserver.cpp shadowsocks.cpp crypto.cpp eventloop.cpp)

target_link_libraries(straydog ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(straydog wsock32 ws2_32)
target_link_libraries(straydog libssl_static libcrypto_static)