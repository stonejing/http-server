
# find_package(Threads REQUIRED)

include_directories("D:/Program/OpenSSL/include")
link_directories("D:/Program/OpenSSL/lib/")

set(SRC_LIST main.cpp proxyserver.cpp shadowsocks.cpp crypto.cpp eventloop.cpp http.cpp)

add_executable(straydog ${SRC_LIST})

target_link_libraries(straydog wsock32 ws2_32)
target_link_libraries(straydog libssl libcrypto)