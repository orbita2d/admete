add_executable(admete main.cpp)

target_link_libraries(admete 
PRIVATE game_library
PRIVATE search_library)

add_subdirectory(game)
add_subdirectory(search)

set_property(TARGET admete PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
set_property(TARGET admete PROPERTY CXX_STANDARD 17)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")


if ( CMAKE_COMPILER_IS_GNUCXX )
    target_compile_options(admete PRIVATE "-Wall" PRIVATE "-Wextra")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    add_subdirectory(analyser)
endif()
if ( MSVC )
    target_compile_options(admete PRIVATE "/W4")
    set(CMAKE_CXX_FLAGS_RELEASE "/O3")
endif()