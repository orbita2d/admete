add_library(search_library 
    search.cpp search.hpp
    perft.cpp 
    transposition.cpp transposition.hpp
    uci.cpp uci.hpp
    ordering.cpp ordering.hpp
    tablebase.cpp tablebase.hpp)

target_link_libraries(search_library game_library fathom)


find_package(Threads)
target_link_libraries(search_library ${CMAKE_THREAD_LIBS_INIT})

set_property(TARGET search_library PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
set_property(TARGET search_library PROPERTY CXX_STANDARD 20)
target_include_directories(search_library PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)


if(USE_AVX2)
    target_compile_definitions(search_library PRIVATE USE_AVX2)
    if(MSVC)
        target_compile_options(search_library PRIVATE "/arch:AVX2")
    elseif(CMAKE_COMPILER_IS_GNUCXX)
        target_compile_options(search_library PRIVATE "-mavx2")
    else()
        message(FATAL_ERROR "AVX2 is not supported on this compiler")
    endif()
endif()

if ( CMAKE_COMPILER_IS_GNUCXX )
target_compile_options(
    search_library 
    PRIVATE "-Wall"
    PRIVATE "-Wextra"
    PRIVATE "-mtune=native"
    PRIVATE "-march=native"
    PRIVATE "-flto" 
    PRIVATE "-ffast-math"
    )
endif()
if ( MSVC )
target_compile_options(search_library PRIVATE "/W4")
endif()