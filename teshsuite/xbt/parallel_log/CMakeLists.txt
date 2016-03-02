add_executable       (parallel_log_crashtest parallel_log_crashtest.c)
target_link_libraries(parallel_log_crashtest simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/parallel_log_crashtest.tesh
  PARENT_SCOPE)
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/parallel_log_crashtest.c
  PARENT_SCOPE)
