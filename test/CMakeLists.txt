add_executable(tests main.cpp
                perft.cpp
                search.cpp)

target_link_libraries(tests
        gtest
        test_library
        game_library
        search_library)

add_test(NAME tests
        COMMAND tests)