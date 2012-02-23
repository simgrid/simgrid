cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(test_MSG_gpu_task_create "test_MSG_gpu_task_create.c")

### Add definitions for compile
target_link_libraries(test_MSG_gpu_task_create simgrid m )

