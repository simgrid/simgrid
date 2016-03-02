add_executable       (parmap_bench parmap_bench.c)
target_link_libraries(parmap_bench simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/parmap_bench.tesh
  PARENT_SCOPE)
set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/parmap_bench.c
  PARENT_SCOPE)
