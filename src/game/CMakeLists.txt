add_library(game_library piece.cpp board.cpp)

target_include_directories(game_library PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)