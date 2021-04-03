add_executable(admete main.cpp)

target_link_libraries(admete 
PRIVATE game_library
PRIVATE search_library)

add_subdirectory(game)
add_subdirectory(search)

set_property(TARGET admete PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
if ( CMAKE_COMPILER_IS_GNUCXX )
    target_compile_options(admete PRIVATE "-Wall" PRIVATE "-Wextra" PRIVATE "-std=c++17")
endif()
if ( MSVC )
    target_compile_options(admete PRIVATE "/W4")
endif()