cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(basic_parsing_test basic_parsing_test.c)
add_executable(is_router_test is_router_test.c)
add_executable(flatifier flatifier.c)
add_executable(graphicator graphicator.c)

### Add definitions for compile
if(NOT WIN32)
target_link_libraries(basic_parsing_test simgrid m pthread )
target_link_libraries(is_router_test simgrid m pthread )
target_link_libraries(flatifier simgrid m pthread )
target_link_libraries(graphicator simgrid m pthread )
else(NOT WIN32)
target_link_libraries(basic_parsing_test simgrid)
target_link_libraries(is_router_test simgrid)
target_link_libraries(flatifier simgrid)
target_link_libraries(graphicator simgrid )
endif(NOT WIN32)

