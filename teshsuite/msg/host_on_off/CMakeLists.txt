foreach(x host_on_off host_on_off_wait host_on_off_recv)
  add_executable       (${x} ${x}.c)
  target_link_libraries(${x} simgrid)

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}.c)

  ADD_TESH_FACTORIES(tesh-msg-${x} "thread;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/msg/host_on_off --cd ${CMAKE_BINARY_DIR}/teshsuite/msg/host_on_off ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/host_on_off/${x}.tesh)
endforeach()

set(teshsuite_src ${teshsuite_src}                                               PARENT_SCOPE)
set(tesh_files    ${tesh_files}                                                  PARENT_SCOPE)
