add_executable(admete-analyser analyser.cpp)

target_link_libraries(admete-analyser 
game_library
search_library)

set_property(TARGET admete-analyser PROPERTY INTERPROCEDURAL_OPTIMIZATION True)

target_compile_options(admete-analyser PRIVATE "-Wall" PRIVATE "-Wextra" PRIVATE "-std=c++17")