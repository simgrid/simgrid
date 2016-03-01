foreach(x host_on_off host_on_off_wait host_on_off_recv)
  add_executable       (${x} ${x}.c)
  target_link_libraries(${x} simgrid)

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}.c)

  ADD_TESH_FACTORIES(tesh-msg-${x} "thread;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/msg/${x} --cd ${CMAKE_BINARY_DIR}/teshsuite/msg/${x} ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/${x}/${x}.tesh)
endforeach()

set(xml_files ${xml_files} ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off_d.xml PARENT_SCOPE)

