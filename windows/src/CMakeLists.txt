
# find_package(Threads REQUIRED)

include_directories("D:/Program/OpenSSL/include")
link_directories("D:/Program/OpenSSL/lib/")

set(SRC_LIST main.cpp server.cpp shadowsocks.cpp crypto.cpp eventloop.cpp http.cpp utils.cpp)

add_executable(server ${SRC_LIST})

target_link_libraries(server wsock32 ws2_32)
# target_link_libraries(server libssl_static libcrypto_static)
target_link_libraries(server libssl libcrypto)