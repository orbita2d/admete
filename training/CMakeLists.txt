add_executable(train main.cpp)

target_link_libraries(train 
PRIVATE game_library)

set_property(TARGET train PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
set_property(TARGET train PROPERTY CXX_STANDARD 20)


if ( CMAKE_COMPILER_IS_GNUCXX )
    target_compile_options(train PRIVATE "-Wall" PRIVATE "-Wextra")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()
if ( MSVC )
    target_compile_options(train PRIVATE "/W4")
    set(CMAKE_CXX_FLAGS_RELEASE "/O3")
endif()