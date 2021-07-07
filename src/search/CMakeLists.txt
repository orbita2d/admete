add_library(search_library 
    search.cpp search.hpp
    perft.cpp 
    transposition.cpp transposition.hpp
    uci.cpp uci.hpp
    ordering.cpp ordering.hpp
    tablebase.cpp tablebase.hpp)

add_subdirectory(fathom)
target_link_libraries(search_library game_library pthread fathom)

set_property(TARGET search_library PROPERTY INTERPROCEDURAL_OPTIMIZATION True)

target_include_directories(search_library PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)



if ( CMAKE_COMPILER_IS_GNUCXX )
target_compile_options(search_library PRIVATE "-Wall" PRIVATE "-Wextra" PRIVATE "-std=c++17")
endif()
if ( MSVC )
target_compile_options(search_library PRIVATE "/W4" PRIVATE "/std=c++17")
endif()