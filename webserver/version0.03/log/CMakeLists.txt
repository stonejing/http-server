cmake_minimum_required(VERSION 3.13)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# add_executable(FileUtilTest FileUtilTest.cpp FileUtil.cpp)
# target_link_libraries(FileUtilTest PRIVATE Threads::Threads)

add_executable(LoggingTest LoggingTest.cpp Logging.cpp LogStream.cpp AsyncLogging.cpp FileUtil.cpp)
target_link_libraries(LoggingTest PRIVATE Threads::Threads)