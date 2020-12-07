foreach(x trace-route-user-variables trace-link-user-variables
        trace-masterworker trace-process-migration)
  if(enable_msg)
    add_executable       (${x}  EXCLUDE_FROM_ALL ${x}/${x}.c)
    target_link_libraries(${x}  simgrid)
    set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
    add_dependencies(tests ${x})
  endif()
  set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
  set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
endforeach()

set(examples_src  ${examples_src}  PARENT_SCOPE)
set(txt_files     ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/README.doc                                  PARENT_SCOPE)
set(tesh_files    ${tesh_files}    PARENT_SCOPE)

if(enable_msg)
  foreach (x trace-route-user-variables trace-link-user-variables trace-masterworker
             trace-process-migration)
    ADD_TESH(msg-${x} --setenv bindir=${CMAKE_BINARY_DIR}/examples/deprecated/msg/${x}
                      --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/${x} 
                      --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms 
                      --cd ${CMAKE_BINARY_DIR}/examples/deprecated/msg/${x}
                      ${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/${x}/${x}.tesh)
  endforeach()
endif(enable_msg)
