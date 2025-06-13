set(WEIGHTS neural/weights.cxx)
set(NN_SOURCES
    neural/features.cpp neural/features.hpp
    neural/linalg.hpp neural/network.hpp
    ${WEIGHTS} neural/weights.hpp
    neural/fixed.hpp
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

target_include_directories(libadmete PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/neural
)

target_compile_definitions(libadmete PRIVATE CACHE_LINE_SIZE=64)
apply_common_target_properties(libadmete)
apply_common_compiler_flags(libadmete)

if(WITH_TUNING)
    target_compile_definitions(libadmete PRIVATE WITH_TUNING)
endif()