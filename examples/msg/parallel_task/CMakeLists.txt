cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/msg/parallel_task/")

add_executable(parallel_task parallel_task.c)
add_executable(test_ptask test_ptask.c)

### Add definitions for compile
target_link_libraries(parallel_task simgrid m )
target_link_libraries(test_ptask simgrid m )
