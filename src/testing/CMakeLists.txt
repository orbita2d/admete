add_library(test_library STATIC
    testing.cpp
    testing.hpp)

target_include_directories(test_library PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)