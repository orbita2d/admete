cmake_minimum_required(VERSION 3.18)
project(admete VERSION 1.9.0
DESCRIPTION "A C++ chess engine written for fun."
LANGUAGES CXX)

option(WITH_PROFILER "Build with gperftools profiler" OFF)
option(WITH_TESTS "Build tests" ON)
option(WITH_TUNING "Allow runtime parameter tuning" OFF)
option(WITH_BINDINGS "Build C style bindings as an SO library" OFF)

set(TARGET_ARCH "native" CACHE STRING "Target architecture for optimization")
set_property(CACHE TARGET_ARCH PROPERTY STRINGS
    "x86-64-v3-vnni" # AVX2 + AVX-VNNI (Alder Lake / Zen 4+); distribution default
    "x86-64-v3"      # AVX2, BMI1/2, FMA — portable fallback, no VNNI (uses dpbusd emulation)
    "native"         # Optimize for build machine (development only)
)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# Code is here


# ============================================================================
# REUSABLE FUNCTIONS FOR COMPILER FLAGS
# ============================================================================

function(apply_common_compiler_flags target_name)
  if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Base warning flags
    target_compile_options(${target_name}
      PRIVATE "-Wall"
      PRIVATE "-Wextra"
    )
    
    # Architecture-specific tuning. AVX2 is a hard baseline (the eval uses _mm256
    # intrinsics unconditionally); the -vnni variant additionally enables AVX-VNNI
    # so dpbusd() lowers to vpdpbusd instead of the maddubs/madd emulation.
    if(TARGET_ARCH STREQUAL "x86-64-v3-vnni")
      target_compile_options(${target_name}
        PRIVATE "-march=x86-64-v3"
        PRIVATE "-mavxvnni"
        PRIVATE "-mtune=generic"
      )
    elseif(TARGET_ARCH STREQUAL "x86-64-v3")
      target_compile_options(${target_name}
        PRIVATE "-march=x86-64-v3"
        PRIVATE "-mtune=generic"
      )
    elseif(TARGET_ARCH STREQUAL "native")
      target_compile_options(${target_name}
        PRIVATE "-march=native"
        PRIVATE "-mtune=native"
      )
    else()
      message(FATAL_ERROR "Unsupported TARGET_ARCH: ${TARGET_ARCH}")
    endif()

  else()
    message(FATAL_ERROR "Only GCC and Clang are currently supported")
  endif()

  set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -flto")
endfunction()

function(apply_common_target_properties target_name)
    set_property(TARGET ${target_name} PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
    set_property(TARGET ${target_name} PROPERTY CXX_STANDARD 23)
endfunction()

# ============================================================================

if(WITH_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
add_subdirectory(src)
add_subdirectory(include)
if(WITH_BINDINGS)
  add_subdirectory(bindings)
endif()
