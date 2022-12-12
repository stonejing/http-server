
# add_library(MathFunctions mysqrt.cppsaqzzxzxrsrgsrsfzdtgjQ)

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

# add_executable(windivert_test windivert_test.cpp)
# target_link_libraries(windivert_test WinDivert)

# enable_testing()
# add_executable(log_test log_test.cpp)
# add_test(NAME log_test1 COMMAND log_test)

# add_executable(vector_test vector_test.cpp)
# add_executable(block_echo_server block_echo_server.cpp)
# add_executable(systray_only WIN32 systray_only.cpp)
