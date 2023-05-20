cmake_minimum_required(VERSION 3.11)

project(linux)

set(CMAKE_EXPORT_COMPILECOMMANDS ON)

# add_executable(dbg_test dbg_test.cpp)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(iteration)