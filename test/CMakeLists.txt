message(STATUS "Building tests")
add_executable(tests main.cpp
        squares.cpp
        perft.cpp
        search.cpp
        ops.cpp
        fen.cpp
        board.cpp
        eval.cpp
        zobrist.cpp
        ordering.cpp
        features.cpp
        linalg.cpp
        network.cpp
        fixed.cpp
        fixed_accumulator.cpp
        )

target_link_libraries(tests
        gtest
        libadmete
        )

add_test(NAME tests
        COMMAND tests)

set_property(TARGET tests PROPERTY CXX_STANDARD 23)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

