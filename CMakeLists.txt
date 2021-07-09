cmake_minimum_required(VERSION 3.18)
project(admete VERSION 1.2
DESCRIPTION "A C++ chess engine written for fun. Contact orbita@orbital.me.uk"
LANGUAGES CXX)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# Code is here

enable_testing()
add_subdirectory(test)
add_subdirectory(src)
add_subdirectory(include)