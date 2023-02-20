if (Eigen3_FOUND)
  set(optional_examples ptask-subflows)
else()
  foreach(x ptask-subflows)
    set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.cpp)
    set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  endforeach()
endif()
foreach(x lmm_usage core_usage core_usage2
          cloud-sharing ptask_L07_usage wifi_usage wifi_usage_decay cm02-set-lat-bw cm02-tcpgamma issue105 ${optional_examples})
  add_executable       (${x}  EXCLUDE_FROM_ALL ${x}/${x}.cpp)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  set_property(TARGET ${x} APPEND PROPERTY INCLUDE_DIRECTORIES "${INTERNAL_INCLUDES}") # wifi tests need it, at least
  add_dependencies(tests ${x})

  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.cpp)
  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)

  ADD_TESH(tesh-model-${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/models/${x} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_BINARY_DIR}/teshsuite/models/${x} ${CMAKE_HOME_DIRECTORY}/teshsuite/models/${x}/${x}.tesh)
endforeach()

# BMF specific tesh tests
foreach(x cm02-set-lat-bw)
  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}-bmf.tesh)

  if(Eigen3_FOUND)
    ADD_TESH(tesh-model-${x}-bmf
             --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/models/${x}
             --setenv srcdir=${CMAKE_CURRENT_SOURCE_DIR}/teshsuite/models/${x}
             --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
             --cd ${CMAKE_BINARY_DIR}/teshsuite/models/${x}
             ${CMAKE_HOME_DIRECTORY}/teshsuite/models/${x}/${x}-bmf.tesh)

  endif()
endforeach()

# Benchmarking MaxMin
add_executable       (maxmin_bench EXCLUDE_FROM_ALL maxmin_bench/maxmin_bench.cpp)
target_link_libraries(maxmin_bench simgrid)
set_target_properties(maxmin_bench PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/maxmin_bench)
set_property(TARGET maxmin_bench APPEND PROPERTY INCLUDE_DIRECTORIES "${INTERNAL_INCLUDES}")
add_dependencies(tests maxmin_bench)
set(teshsuite_src  ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/maxmin_bench/maxmin_bench.cpp)
foreach(x small medium large)
  set(tesh_files     ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/maxmin_bench/maxmin_bench_${x}.tesh)
  ADD_TESH(tesh-maxmin-${x} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/models/maxmin_bench --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/models/maxmin_bench maxmin_bench_${x}.tesh)
endforeach()

set(teshsuite_src ${teshsuite_src}  PARENT_SCOPE)
set(tesh_files    ${tesh_files}     PARENT_SCOPE)
