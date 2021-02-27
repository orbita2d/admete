add_library(search_library search.cpp search.hpp perft.cpp)

target_include_directories(search_library PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)