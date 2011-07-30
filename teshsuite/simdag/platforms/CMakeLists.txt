cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(basic_parsing_test basic_parsing_test.c)
add_executable(is_router_test is_router_test.c)
add_executable(flatifier flatifier.c)
if(NOT WIN32 AND NOT APPLE)
add_executable(evaluate_parse_time Evaluate_parse_time.c)
add_executable(evaluate_get_route_time Evaluate_get_route_time.c)
endif(NOT WIN32 AND NOT APPLE)

### Add definitions for compile
if(NOT WIN32 AND NOT APPLE)
target_link_libraries(evaluate_get_route_time simgrid m)
target_link_libraries(evaluate_parse_time simgrid m)
target_link_libraries(basic_parsing_test simgrid m)
target_link_libraries(is_router_test simgrid m)
target_link_libraries(flatifier simgrid m)
else(NOT WIN32 AND NOT APPLE)
target_link_libraries(basic_parsing_test simgrid)
target_link_libraries(is_router_test simgrid)
target_link_libraries(flatifier simgrid)
endif(NOT WIN32 AND NOT APPLE)