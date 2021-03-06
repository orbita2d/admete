add_library(search_library search.cpp search.hpp perft.cpp evaluate.cpp)

target_link_libraries(search_library game_library)

target_include_directories(search_library PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)