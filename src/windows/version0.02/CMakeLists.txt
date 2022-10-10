project(straydog)
cmake_MINIMUM_REQUIRED(VERSION 3.11)

set(MY_TARGET straydog)

AUX_SOURCE_DIRECTORY(. SRC_LIST)

add_executable(${MY_TARGET} ${SRC_LIST})

