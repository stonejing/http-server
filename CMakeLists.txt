cmake_minimum_required(VERSION 3.14.2)
project(HTTP-SERVER)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

add_definitions(-DDebug=1)

if (UNIX)
    add_subdirectory(linux)
endif (UNIX)

if (WIN32)
    
endif (WIN32)

if (MSVC)
    add_subdirectory(windows)
endif (MSVC)