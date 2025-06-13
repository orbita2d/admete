cmake_minimum_required(VERSION 3.18)
project(admete VERSION 1.8.0
DESCRIPTION "A C++ chess engine written for fun."
LANGUAGES CXX)

option(WITH_PROFILER "Build with gperftools profiler" OFF)
option(USE_AVX2 "Enable AVX2 instructions" OFF)
option(WITH_TESTS "Build tests" ON)
option(WITH_TUNING "Allow runtime parameter tuning" OFF)
option(WITH_BINDINGS "Build C style bindings as an SO library" OFF)

set(TARGET_ARCH "native" CACHE STRING "Target architecture for optimization")
set_property(CACHE TARGET_ARCH PROPERTY STRINGS 
    "x86-64-v3"     # AVX2, BMI1, BMI2, FMA (2013+ Intel, 2015+ AMD) 
    "skylake"       # Intel Skylake (2015) - adds more features
    "zen3"          # AMD Zen 3 (2020) - improved IPC
    "native"        # Optimize for build machine (development only)
)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# Code is here


# ============================================================================
# REUSABLE FUNCTIONS FOR COMPILER FLAGS
# ============================================================================

function(apply_common_compiler_flags target_name)
  if(CMAKE_COMPILER_IS_GNUCXX)
    # Base warning flags
    target_compile_options(${target_name}
      PRIVATE "-Wall"
      PRIVATE "-Wextra"
    )
    
    # Architecture-specific tuning
    if(TARGET_ARCH STREQUAL "x86-64-v3")
      target_compile_options(${target_name} 
        PRIVATE "-march=x86-64-v3" 
        PRIVATE "-mtune=generic"
      )
    elseif(TARGET_ARCH STREQUAL "skylake")
      target_compile_options(${target_name} 
        PRIVATE "-march=skylake" 
        PRIVATE "-mtune=skylake"
      )
    elseif(TARGET_ARCH STREQUAL "zen3")
      target_compile_options(${target_name} 
        PRIVATE "-march=znver3" 
        PRIVATE "-mtune=znver3"
      )
    elseif(TARGET_ARCH STREQUAL "native")
      target_compile_options(${target_name} 
        PRIVATE "-march=native" 
        PRIVATE "-mtune=native"
      )
    else()
      message(FATAL_ERROR "Unsupported TARGET_ARCH: ${TARGET_ARCH}")
    endif()
    
    # Handle AVX2 option
    if(USE_AVX2)
      target_compile_options(${target_name} PRIVATE "-mavx2")
    endif()
    
  else()
    message(FATAL_ERROR "Only GCC is currently supported")
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
