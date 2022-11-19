# SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++17 -pthread")

find_package(Threads REQUIRED)

include_directories("D:/Program/OpenSSL/include")
link_directories("D:/Program/OpenSSL/lib/")

set(SRC_LIST main.cpp straydog.cpp proxyserver.cpp shadowsocks.cpp crypto.cpp eventloop.cpp)

add_executable(straydog ${SRC_LIST})

target_link_libraries(straydog wsock32 ws2_32)
target_link_libraries(straydog libssl_static libcrypto_static)