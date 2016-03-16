foreach(x concurrent_rw get_sender host_on_off host_on_off_processes host_on_off_recv host_on_off_wait listen_async pid
          process process_join storage_client_server task_destroy_cancel)
  add_executable       (${x}  ${x}/${x}.c)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)

  ADD_TESH_FACTORIES(tesh-msg-${x} "thread;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/msg/${x} --cd ${CMAKE_BINARY_DIR}/teshsuite/msg/${x} ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/${x}/${x}.tesh)
endforeach()

set(teshsuite_src ${teshsuite_src}  PARENT_SCOPE)
set(tesh_files    ${tesh_files}     PARENT_SCOPE)
