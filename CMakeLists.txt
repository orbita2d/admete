cmake_minimum_required(VERSION 3.18)
project(admete VERSION 1.6.0
DESCRIPTION "A C++ chess engine written for fun. Contact orbita@orbital.me.uk"
LANGUAGES CXX)

option(WITH_PROFILER "Build with gperftools profiler" OFF)
option(USE_AVX2 "Enable AVX2 instructions" OFF)
option(WITH_TESTS "Build tests" ON)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# Code is here

if(WITH_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
add_subdirectory(src)
add_subdirectory(include)