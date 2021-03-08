add_library(game_library piece.cpp
            board.cpp
            printing.cpp
            movegen.cpp
            bitboard.cpp)

set_property(TARGET game_library PROPERTY INTERPROCEDURAL_OPTIMIZATION True)

target_include_directories(game_library PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)