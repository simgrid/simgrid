add_executable       (log_large_test log_large_test.c)
target_link_libraries(log_large_test simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/log_large_test.tesh
  PARENT_SCOPE)
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/log_large_test.c
  PARENT_SCOPE)
