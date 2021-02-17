add_executable(admete main.cpp)

target_link_libraries(admete 
PRIVATE game_library)

add_subdirectory(game)

if ( CMAKE_COMPILER_IS_GNUCXX )
    target_compile_options(admete PRIVATE "-Wall" PRIVATE "-Wextra")
endif()
if ( MSVC )
    target_compile_options(admete PRIVATE "/W4")
endif()