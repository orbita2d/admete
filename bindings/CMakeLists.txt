add_library(admete_bindings SHARED
    api.cpp
    api.h
)

target_link_libraries(admete_bindings 
    PRIVATE libadmete
)

set_property(TARGET admete_bindings PROPERTY CXX_STANDARD 23)
set_property(TARGET admete_bindings PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(admete_bindings PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(admete_bindings PROPERTIES
    OUTPUT_NAME "admete"
)

if ( CMAKE_COMPILER_IS_GNUCXX )
    target_compile_options(admete_bindings
        PRIVATE "-Wall"
        PRIVATE "-Wextra"
        PRIVATE "-mtune=native"
        PRIVATE "-march=native"
    )
    set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -flto")
endif()
if ( MSVC )
    target_compile_options(admete_bindings PRIVATE "/W4")
    set(CMAKE_CXX_FLAGS_RELEASE "/O3 /fp:fast /GL")
endif()