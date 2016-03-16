foreach(x availability comm-mxn-all2all comm-mxn-independent comm-mxn-scatter comm-p2p-latency-1 comm-p2p-latency-2
          comm-p2p-latency-3 comm-p2p-latency-bound incomplete)
  add_executable       (${x}  ${x}/${x}.c)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)

  ADD_TESH(tesh-simdag-${x} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simdag/${x} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/${x} ${x}.tesh)
endforeach()

set(teshsuite_src ${teshsuite_src}  PARENT_SCOPE)
set(tesh_files    ${tesh_files}     PARENT_SCOPE)

