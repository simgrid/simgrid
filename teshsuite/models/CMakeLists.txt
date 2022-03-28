if (Eigen3_FOUND)
  set(optional_examples ptask-subflows)
else()
  foreach(x ptask-subflows)
    set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.cpp)
    set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  endforeach()
endif()
foreach(x cloud-sharing ptask_L07_usage wifi_usage wifi_usage_decay cm02-set-lat-bw issue105 ${optional_examples})
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


set(teshsuite_src ${teshsuite_src}  PARENT_SCOPE)
set(tesh_files    ${tesh_files}     PARENT_SCOPE)
