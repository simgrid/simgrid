foreach(x get_sender task_listen_from task_destroy_cancel)
  if(enable_msg)
    add_executable       (${x} EXCLUDE_FROM_ALL ${x}/${x}.c)
    target_link_libraries(${x} simgrid)
    set_target_properties(${x} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
    add_dependencies(tests ${x})
  endif()

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
endforeach()

set(teshsuite_src ${teshsuite_src}  PARENT_SCOPE)
set(tesh_files    ${tesh_files}     PARENT_SCOPE)

if(enable_msg)
  foreach(x get_sender task_destroy_cancel task_listen_from)

    ADD_TESH_FACTORIES(tesh-msg-${x} "raw"  --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                                            --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/msg/${x}
                                            --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/${x} 
                                            ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/${x}/${x}.tesh)
  endforeach()
endif()
