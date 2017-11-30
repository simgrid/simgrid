foreach(x lmm_usage surf_usage surf_usage2)
  add_executable       (${x}  ${x}/${x}.cpp)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.cpp)
endforeach()

add_executable       (maxmin_bench maxmin_bench/maxmin_bench.cpp)
target_link_libraries(maxmin_bench simgrid)
set_target_properties(maxmin_bench PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/maxmin_bench)

foreach(x small medium large)
  set(tesh_files     ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/maxmin_bench/maxmin_bench_${x}.tesh)
endforeach()

set(tesh_files     ${tesh_files}                                                               PARENT_SCOPE)
set(teshsuite_src  ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/maxmin_bench/maxmin_bench.cpp  PARENT_SCOPE)

foreach(x lmm_usage surf_usage surf_usage2)
  ADD_TESH(tesh-surf-${x} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/surf/${x} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/surf/${x} ${x}.tesh)
endforeach()

foreach(x small medium large)
  ADD_TESH(tesh-surf-maxmin-${x} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/surf/maxmin_bench --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/surf/maxmin_bench maxmin_bench_${x}.tesh)
endforeach()
