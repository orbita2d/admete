cmake_minimum_required(VERSION 3.18)
project(admete VERSION 0.1
DESCRIPTION "Just another chess engine"
LANGUAGES CXX)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# Code is here
add_subdirectory(src)