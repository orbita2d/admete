add_library(fathom tbprobe.cpp)

set_property(TARGET fathom PROPERTY INTERPROCEDURAL_OPTIMIZATION True)

target_include_directories(fathom PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)