cmake_minimum_required(VERSION 3.8.0)

project(blockHasher)

add_executable( blockHasher
                block_hasher.cpp
                md5.cpp
                main.cpp)

target_include_directories(blockHasher PRIVATE ./)
target_compile_options(blockHasher PRIVATE -std=c++1z -O2 -Wall -Werror -Wextra -Wno-unused-variable)
