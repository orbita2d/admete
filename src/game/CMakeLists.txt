set(NN_SOURCES
    neural/features.cpp neural/features.hpp
    neural/linalg.hpp neural/network.hpp
    neural/weights.cpp neural/weights.hpp
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


if(USE_AVX2)
    target_compile_definitions(game_library PRIVATE USE_AVX2)
    if(MSVC)
        target_compile_options(game_library PRIVATE "/arch:AVX2")
    elseif(CMAKE_COMPILER_IS_GNUCXX)
        target_compile_options(game_library PRIVATE "-mavx2")
    else()
        message(FATAL_ERROR "AVX2 is not supported on this compiler")
    endif()
endif()

if ( CMAKE_COMPILER_IS_GNUCXX )
target_compile_options(
    game_library 
    PRIVATE "-Wall"
    PRIVATE "-Wextra"
    PRIVATE "-mtune=native"
    PRIVATE "-march=native"
    PRIVATE "-flto" 
    PRIVATE "-ffast-math"
    )
endif()
if ( MSVC )
target_compile_options(game_library PRIVATE "/W4")
endif()