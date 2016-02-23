add_executable       (maxmin_bench maxmin_bench.c)
target_link_libraries(maxmin_bench simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/maxmin_bench.tesh
  PARENT_SCOPE)
set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/maxmin_bench.c
  PARENT_SCOPE)
