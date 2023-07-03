cmake_minimum_required(VERSION 3.14.2)
project(HTTP-SERVER)

if (UNIX)
    add_subdirectory(linux)
endif (UNIX)

if (WIN32)
    
endif (WIN32)

if (MSVC)
    add_subdirectory(windows)
endif (MSVC)