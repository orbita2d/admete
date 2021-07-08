add_executable(admete-analyser analyser.cpp)

target_link_libraries(admete-analyser 
PRIVATE game_library
PRIVATE search_library
PRIVATE pthread)

set_property(TARGET admete-analyser PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
set_property(TARGET admete-analyser PROPERTY CXX_STANDARD 17)

if ( CMAKE_COMPILER_IS_GNUCXX )
target_compile_options(admete-analyser PRIVATE "-Wall" PRIVATE "-Wextra")
endif()
if ( MSVC )
target_compile_options(admete-analyser PRIVATE "/W4")
endif()