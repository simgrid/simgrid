cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

add_executable(gras_stub_generator stub_generator.c unix_stub_generator.c windows_stub_generator.c)

### Add definitions for compile
if(NOT WIN32)
    target_link_libraries(gras_stub_generator gras simgrid pthread m)
else(NOT WIN32)
    target_link_libraries(gras_stub_generator simgrid gras)
endif(NOT WIN32)