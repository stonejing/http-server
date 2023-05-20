add_executable(hello_world hello_world.cpp)
find_package(Threads REQUIRED)
target_link_libraries(hello_world PUBLIC "-pthread")

add_executable(thread_local_data thread_local_data.cpp)
target_link_libraries(thread_local_data PUBLIC "-pthread")

add_executable(mutex_lock mutex_lock.cpp)
target_link_libraries(mutex_lock PUBLIC "-pthread")

add_executable(condition_variable condition_variable.cpp)
target_link_libraries(condition_variable PUBLIC "-pthread")

add_executable(fork_test fork_test.cpp)
