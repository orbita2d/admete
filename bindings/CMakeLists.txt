add_library(admete_bindings SHARED
    api.cpp
    api.h
)

target_link_libraries(admete_bindings 
    PRIVATE libadmete
)

target_include_directories(admete_bindings PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(admete_bindings PROPERTIES
    OUTPUT_NAME "admete"
)

apply_common_target_properties(admete_bindings)
apply_common_compiler_flags(admete_bindings)