add_executable(tests main.cpp
                perft.cpp
                search.cpp
                ops.cpp
                fen.cpp)

target_link_libraries(tests
        gtest
        game_library
        search_library)

add_test(NAME tests
        COMMAND tests)

target_compile_options(tests PRIVATE "-std=c++17")