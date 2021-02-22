add_library(search_library search.cpp search.hpp)

target_include_directories(search_library PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)