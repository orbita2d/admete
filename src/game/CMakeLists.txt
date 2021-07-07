add_library(game_library types.hpp
            board.cpp board.hpp
            printing.cpp printing.hpp
            movegen.cpp movegen.hpp
            bitboard.cpp bitboard.hpp
            evaluate.cpp evaluate.hpp
            zobrist.cpp)

set_property(TARGET game_library PROPERTY INTERPROCEDURAL_OPTIMIZATION True)

target_include_directories(game_library PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if ( CMAKE_COMPILER_IS_GNUCXX )
target_compile_options(game_library PRIVATE "-Wall" PRIVATE "-Wextra" PRIVATE "-std=c++17" PRIVATE "-static-libgcc" PRIVATE "-static-libstdc++")
endif()
if ( MSVC )
target_compile_options(game_library PRIVATE "/W4" PRIVATE "/std=c++17")
endif()