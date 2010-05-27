cmake_minimum_required(VERSION 2.6)

#set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/src/")
set(USE_TEST_UNITS
${TEST_UNITS}
${PROJECT_DIRECTORY}/src/simgrid_units_main.c
)

set_source_files_properties(${USE_TEST_UNITS}
PROPERTIES GENERATED true)

add_executable(testall ${USE_TEST_UNITS})

### Add definitions for compile
target_link_libraries(testall gras m -fprofile-arcs)
