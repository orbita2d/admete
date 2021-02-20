add_executable(testperft main.cpp
                perft.cpp)

target_link_libraries(testperft
        gtest
        test_library
        game_library)

add_test(NAME testperft
        COMMAND testperft)