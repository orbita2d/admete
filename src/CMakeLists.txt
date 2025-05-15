add_executable(admete main.cpp)

target_link_libraries(admete 
PRIVATE libadmete)

add_subdirectory(admete)

set_property(TARGET admete PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
set_property(TARGET admete PROPERTY CXX_STANDARD 23)

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

if(WITH_PROFILER)
    find_library(GPERFTOOLS_PROFILER profiler REQUIRED)
    target_link_libraries(admete PRIVATE ${GPERFTOOLS_PROFILER})
    target_compile_definitions(admete PRIVATE WITH_GPERFTOOLS)
endif()