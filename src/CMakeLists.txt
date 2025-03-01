add_executable(admete main.cpp)

target_link_libraries(admete 
PRIVATE game_library
PRIVATE search_library)

add_subdirectory(game)
add_subdirectory(search)

set_property(TARGET admete PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
set_property(TARGET admete PROPERTY CXX_STANDARD 20)

option(WITH_PROFILER "Build with gperftools profiler" OFF)
option(USE_AVX2 "Enable AVX2 instructions" OFF)
# option(CACHE_LINE_SIZE "Size of cache line in bytes" 64)

if ( CMAKE_COMPILER_IS_GNUCXX )
    target_compile_options(admete
        PRIVATE "-Wall"
        PRIVATE "-Wextra"
        PRIVATE "-mtune=native"
        PRIVATE "-march=native"
    )
    set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -flto")
endif()
if ( MSVC )
    target_compile_options(admete PRIVATE "/W4")
    set(CMAKE_CXX_FLAGS_RELEASE "/O3 /fp:fast /GL")
endif()


if(USE_AVX2)
    message(STATUS "Enabling AVX2 instructions")
    target_compile_definitions(admete PRIVATE USE_AVX2)
    if(MSVC)
        add_compile_options(/arch:AVX2)
    elseif(CMAKE_COMPILER_IS_GNUCXX)
        target_compile_options(admete PRIVATE "-mavx2")
    else()
        message(FATAL_ERROR "AVX2 is not supported on this compiler")
    endif()
else()
    message(STATUS "Using native vectorisations")
endif()

if(WITH_PROFILER)
    find_library(GPERFTOOLS_PROFILER profiler REQUIRED)
    target_link_libraries(admete PRIVATE ${GPERFTOOLS_PROFILER})
    target_compile_definitions(admete PRIVATE WITH_GPERFTOOLS)
endif()