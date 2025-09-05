cmake_minimum_required(VERSION 3.13)

project(redis-starter-cpp)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
set(THREADS_PREFER_PTHREAD_FLAG ON)

#find_package(Threads REQUIRED)
# Ran below commands
# git clone https://github.com/microsoft/vcpkg "$HOME/vcpkg"
# ./bootstrap.sh which created vcpkg in same dir
# /home/punit/vcpkg/vcpkg install in source root dir
# setting 'export CMAKE_PREFIX_PATH=/home/punit/vcpkg/packages/asio_x64-linux/share/asio'
# cmake .. in build directory
# cmake --build . in build directory
# It seems vcpkg stores built binaries under /home/punit/.cache/vcpkg/archives/*
#find_package(asio CONFIG REQUIRED)

add_executable(server ${SOURCE_FILES})

#target_link_libraries(server PRIVATE asio asio::asio)
#target_link_libraries(server PRIVATE Threads::Threads)

