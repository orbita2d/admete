add_executable(admete main.cpp)

target_link_libraries(admete 
PRIVATE libadmete)

add_subdirectory(admete)

set_property(TARGET admete PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
set_property(TARGET admete PROPERTY CXX_STANDARD 23)

apply_common_target_properties(admete)
apply_common_compiler_flags(admete)

if(WITH_PROFILER)
    find_library(GPERFTOOLS_PROFILER profiler REQUIRED)
    target_link_libraries(admete PRIVATE ${GPERFTOOLS_PROFILER})
    target_compile_definitions(admete PRIVATE WITH_GPERFTOOLS)
endif()

if(WITH_TUNING)
    target_compile_definitions(admete PRIVATE WITH_TUNING)
    # print that we are using tuning
    message(STATUS "Using tuning")
endif()