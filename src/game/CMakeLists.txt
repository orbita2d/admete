set(NN_SOURCES
    neural/features.cpp neural/features.hpp
    neural/linalg.hpp
    neural/network.cpp neural/network.hpp
)

add_library(game_library types.hpp
            board.cpp board.hpp
            printing.cpp printing.hpp
            movegen.cpp movegen.hpp
            bitboard.cpp bitboard.hpp
            evaluate.cpp evaluate.hpp constants.cpp
            zobrist.cpp zobrist.hpp
            cache.cpp cache.hpp
            ${NN_SOURCES}
            )

set_property(TARGET game_library PROPERTY INTERPROCEDURAL_OPTIMIZATION True)

target_include_directories(game_library PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/neural
)

set_property(TARGET game_library PROPERTY CXX_STANDARD 20)

if ( CMAKE_COMPILER_IS_GNUCXX )
target_compile_options(game_library PRIVATE "-Wall" PRIVATE "-Wextra")
endif()
if ( MSVC )
target_compile_options(game_library PRIVATE "/W4")
endif()