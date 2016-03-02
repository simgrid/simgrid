add_executable       (maxmin_bench maxmin_bench.cpp)
target_link_libraries(maxmin_bench simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/maxmin_bench_small.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/maxmin_bench_medium.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/maxmin_bench_large.tesh
  PARENT_SCOPE)
set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/maxmin_bench.cpp
  PARENT_SCOPE)
