cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

add_executable(gras_stub_generator stub_generator.c unix_stub_generator.c windows_stub_generator.c)
add_executable(struct_diff struct_diff.c)

### Add definitions for compile
if(NOT WIN32)
    target_link_libraries(gras_stub_generator simgrid pthread m)
    target_link_libraries(struct_diff         simgrid pthread m)
else(NOT WIN32)
    target_link_libraries(gras_stub_generator simgrid)
    target_link_libraries(struct_diff         simgrid)
endif(NOT WIN32)