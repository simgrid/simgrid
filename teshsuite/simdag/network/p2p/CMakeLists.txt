foreach(x p2p-latency1 p2p-latency2 p2p-latency3 p2p-latency_bound)
  add_executable       (${x} ${x}.c)
  target_link_libraries(${x} simgrid)

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}.c)

  ADD_TESH(tesh-simdag-${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite --cd ${CMAKE_BINARY_DIR}/teshsuite ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/network/p2p/${x}.tesh)
endforeach()

set(teshsuite_src ${teshsuite_src}                                                     PARENT_SCOPE)
set(tesh_files    ${tesh_files}                                                        PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/platform_2p_1bb.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/platform_2p_1fl.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/platform_2p_1sl.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/platform_2p_1switch.xml PARENT_SCOPE)
