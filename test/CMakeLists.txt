add_executable(tests main.cpp
                perft.cpp
                search.cpp
                ops.cpp
                fen.cpp
                board.cpp
                eval.cpp
                zobrist.cpp
                ordering.cpp)

target_link_libraries(tests
        gtest
        game_library
        search_library)

add_test(NAME tests
        COMMAND tests)

set_property(TARGET tests PROPERTY CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

