add_executable       (maxmin_bench maxmin_bench.c)
target_link_libraries(maxmin_bench simgrid)

foreach(x small medium large)
  set(tesh_files     ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/maxmin_bench_${x}.tesh)
  ADD_TESH(tesh-surf-maxmin-${x} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/surf/maxmin_bench --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/surf/maxmin_bench maxmin_bench_${x}.tesh)
endforeach()

set(tesh_files     ${tesh_files}                                                  PARENT_SCOPE)
set(testsuite_src  ${testsuite_src}  ${CMAKE_CURRENT_SOURCE_DIR}/maxmin_bench.c   PARENT_SCOPE)
