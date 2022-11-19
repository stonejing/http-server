
# add_library(MathFunctions mysqrt.cpp)

# add_executable(mysqrt mysqrt.cpp)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories("D:/Program/OpenSSL/include")
include_directories("D:/Program/WinDivert-2.2.1-A/include")


link_directories("D:/Program/OpenSSL/lib/")
link_directories("D:/Program/WinDivert-2.2.1-A/x64")

add_executable(cmake_test cmake_test.cpp)
target_link_libraries(cmake_test ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(cmake_test wsock32 ws2_32)
target_link_libraries(cmake_test libssl_static libcrypto_static)

add_executable(windivert_test windivert_test.cpp)
target_link_libraries(windivert_test WinDivert)

add_executable(mysqrt_test mysqrt.cpp)