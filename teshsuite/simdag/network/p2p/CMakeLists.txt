cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(test_latency1 test_latency1.c)
add_executable(test_latency2 test_latency2.c)
add_executable(test_latency3 test_latency3.c)
add_executable(test_latency_bound test_latency_bound.c)

### Add definitions for compile
if(NOT WIN32)
target_link_libraries(test_latency1 simgrid m pthread )
target_link_libraries(test_latency2 simgrid m pthread )
target_link_libraries(test_latency3 simgrid m pthread )
target_link_libraries(test_latency_bound simgrid m pthread )
else(NOT WIN32)
target_link_libraries(test_latency1 simgrid)
target_link_libraries(test_latency2 simgrid)
target_link_libraries(test_latency3 simgrid)
target_link_libraries(test_latency_bound simgrid)
endif(NOT WIN32)
