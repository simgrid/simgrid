foreach (x centralized_mutex bugged2_liveness)
  if(SIMGRID_HAVE_MC AND SIMGRID_HAVE_MSG)
    add_executable       (${x} EXCLUDE_FROM_ALL ${x}.c)
    target_link_libraries(${x} simgrid)
    add_dependencies(tests ${x})
  endif()
  set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}.c)
  set(xml_files     ${xml_files}    ${CMAKE_CURRENT_SOURCE_DIR}/deploy_${x}.xml)
endforeach()

set(tesh_files   ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/bugged2-liveness.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/centralized_mutex.tesh                PARENT_SCOPE)
set(xml_files    ${xml_files}                                                                       PARENT_SCOPE)
set(examples_src ${examples_src}                                                                    PARENT_SCOPE)
set(bin_files    ${bin_files}     ${CMAKE_CURRENT_SOURCE_DIR}/promela_bugged2_liveness              PARENT_SCOPE)

#if(SIMGRID_HAVE_MC)
#  IF(HAVE_UCONTEXT_CONTEXTS AND SIMGRID_PROCESSOR_x86_64) # liveness model-checking works only on 64bits (for now ...)
#    ADD_TESH(mc-bugged2-liveness-ucontext     --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}
#                                              --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
#                                              --cd ${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/mc
#                                              bugged2-liveness.tesh)
#  ENDIF()
#ENDIF()