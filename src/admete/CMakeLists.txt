set(NN_SOURCES
    neural/features.cpp neural/features.hpp
    neural/linalg.hpp neural/network.hpp
    neural/weights.cpp neural/weights.hpp
)

set(SEARCH_SOURCES
    search.cpp search.hpp
    perft.cpp 
    transposition.cpp transposition.hpp
    uci.cpp uci.hpp
    ordering.cpp ordering.hpp
    tablebase.cpp tablebase.hpp
)

add_library(
    libadmete types.hpp
    board.cpp board.hpp
    printing.cpp printing.hpp
    movegen.cpp movegen.hpp
    bitboard.cpp bitboard.hpp
    evaluate.cpp evaluate.hpp
    zobrist.cpp zobrist.hpp
    ${NN_SOURCES}
    ${SEARCH_SOURCES}
    )

target_link_libraries(libadmete fathom)

# Find the Threads library
find_package(Threads)
target_link_libraries(libadmete ${CMAKE_THREAD_LIBS_INIT})

set_property(TARGET libadmete PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
set_property(TARGET libadmete PROPERTY CXX_STANDARD 20)

target_include_directories(libadmete PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/neural
)

target_compile_definitions(libadmete PRIVATE CACHE_LINE_SIZE=64)

if(USE_AVX2)
    if(MSVC)
        target_compile_options(libadmete PRIVATE "/arch:AVX2")
    elseif(CMAKE_COMPILER_IS_GNUCXX)
        target_compile_options(libadmete PRIVATE "-mavx2")
    else()
        message(FATAL_ERROR "AVX2 is not supported on this compiler")
    endif()
endif()

if ( CMAKE_COMPILER_IS_GNUCXX )
target_compile_options(
    libadmete 
    PRIVATE "-Wall"
    PRIVATE "-Wextra"
    PRIVATE "-mtune=native"
    PRIVATE "-march=native"
    )
endif()
if ( MSVC )
target_compile_options(libadmete PRIVATE "/W4")
endif()