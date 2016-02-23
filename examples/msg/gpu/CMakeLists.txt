add_executable       (test_MSG_gpu_task_create test_MSG_gpu_task_create.c)
target_link_libraries(test_MSG_gpu_task_create simgrid)

set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_MSG_gpu_task_create.c
  PARENT_SCOPE)
