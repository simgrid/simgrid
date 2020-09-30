foreach(x lmm_usage surf_usage surf_usage2 wifi_usage wifi_usage_decay)
  add_executable       (${x}  EXCLUDE_FROM_ALL ${x}/${x}.cpp)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  set_property(TARGET ${x} APPEND PROPERTY INCLUDE_DIRECTORIES "${INTERNAL_INCLUDES}")
  add_dependencies(tests ${x})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.cpp)

  ADD_TESH(tesh-surf-${x} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/surf/${x} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/surf/${x} ${x}.tesh)
endforeach()

add_executable       (maxmin_bench EXCLUDE_FROM_ALL maxmin_bench/maxmin_bench.cpp)
target_link_libraries(maxmin_bench simgrid)
set_target_properties(maxmin_bench PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/maxmin_bench)
set_property(TARGET maxmin_bench APPEND PROPERTY INCLUDE_DIRECTORIES "${INTERNAL_INCLUDES}")
add_dependencies(tests maxmin_bench)

foreach(x small medium large)
  set(tesh_files     ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/maxmin_bench/maxmin_bench_${x}.tesh)
endforeach()

set(tesh_files     ${tesh_files}                                                               PARENT_SCOPE)
set(teshsuite_src  ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/maxmin_bench/maxmin_bench.cpp  PARENT_SCOPE)

ADD_TESH(tesh-surf-maxmin-large --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/surf/maxmin_bench --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/surf/maxmin_bench maxmin_bench_large.tesh)

if(enable_debug)
  foreach(x small medium)
    ADD_TESH(tesh-surf-maxmin-${x} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/surf/maxmin_bench --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/surf/maxmin_bench maxmin_bench_${x}.tesh)
  endforeach()
endif()