add_executable(admete main.cpp)

target_link_libraries(admete 
PRIVATE game_library
PRIVATE search_library)

add_subdirectory(game)
add_subdirectory(search)

set_property(TARGET admete PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
set_property(TARGET admete PROPERTY CXX_STANDARD 20)


if ( CMAKE_COMPILER_IS_GNUCXX )
    target_compile_options(admete
        PRIVATE "-Wall"
        PRIVATE "-Wextra"
        PRIVATE "-march=native" 
        PRIVATE "-mtune=native" 
        PRIVATE "-flto" 
    )
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto")
endif()
if ( MSVC )
    target_compile_options(admete PRIVATE "/W4")
    set(CMAKE_CXX_FLAGS_RELEASE "/O3")
endif()

option(WITH_PROFILER "Build with gperftools profiler" OFF)

if(WITH_PROFILER)
    find_library(GPERFTOOLS_PROFILER profiler REQUIRED)
    target_link_libraries(admete PRIVATE ${GPERFTOOLS_PROFILER})
    target_compile_definitions(admete PRIVATE WITH_GPERFTOOLS)
endif()