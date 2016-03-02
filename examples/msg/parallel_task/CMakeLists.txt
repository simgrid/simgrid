add_executable       (parallel_task parallel_task.c)
target_link_libraries(parallel_task simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/parallel_task.tesh
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/parallel_task.c
  PARENT_SCOPE)
