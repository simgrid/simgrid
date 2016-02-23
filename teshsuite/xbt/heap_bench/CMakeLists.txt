add_executable       (heap_bench heap_bench.c)
target_link_libraries(heap_bench simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/heap_bench.tesh
  PARENT_SCOPE)
set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/heap_bench.c
  PARENT_SCOPE)
