
add_executable(straydog straydog.cpp aead_encrypt.cpp dbg.cpp)

set(OPENSSL_INCLUDE_DIR "D:\\Qt\\Tools\\OpenSSL\\Win_x64\\include")
set(OPENSSL_LIBRARIES "D:\\Qt\\Tools\\OpenSSL\\Win_x64\\lib")

target_include_directories(straydog PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_directories(straydog PRIVATE ${OPENSSL_LIBRARIES})
target_link_libraries(straydog PRIVATE ws2_32 libcrypto libssl)
