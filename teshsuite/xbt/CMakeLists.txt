cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(log_large_test log_large_test.c)
add_executable(parallel_log_crashtest parallel_log_crashtest.c)

### Add definitions for compile
if(NOT WIN32)
target_link_libraries(log_large_test gras m pthread )
target_link_libraries(parallel_log_crashtest gras m pthread )
else(NOT WIN32)
target_link_libraries(log_large_test gras)
target_link_libraries(parallel_log_crashtest gras)
endif(NOT WIN32)
