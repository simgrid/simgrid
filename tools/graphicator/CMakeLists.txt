cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

add_executable(graphicator graphicator.c)

### Add definitions for compile
if(NOT WIN32)
target_link_libraries(graphicator simgrid m pthread )
else(NOT WIN32)
target_link_libraries(graphicator simgrid )
endif(NOT WIN32)

