foreach(x mxn-all2all mxn-independent_comm mxn-scatter)
  add_executable       (${x} ${x}.c)
  target_link_libraries(${x} simgrid)

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}.c)

  ADD_TESH(tesh-simdag-${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite --cd ${CMAKE_BINARY_DIR}/teshsuite ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/network/mxn/${x}.tesh)
endforeach()

set(teshsuite_src ${teshsuite_src}                                                     PARENT_SCOPE)
set(tesh_files    ${tesh_files}                                                        PARENT_SCOPE)
set(xml_files     ${xml_files}    ${CMAKE_CURRENT_SOURCE_DIR}/platform_4p_1switch.xml  PARENT_SCOPE)
