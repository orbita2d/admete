add_library(search_library search.cpp search.hpp perft.cpp evaluate.cpp evaluate.hpp)

target_link_libraries(search_library game_library)

set_property(TARGET search_library PROPERTY INTERPROCEDURAL_OPTIMIZATION True)

target_include_directories(search_library PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)