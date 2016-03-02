add_executable       (log_usage log_usage.c)
target_link_libraries(log_usage simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/log_usage.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/log_usage_ndebug.tesh
  PARENT_SCOPE)
set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/log_usage.c
  PARENT_SCOPE)
