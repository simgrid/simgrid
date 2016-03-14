foreach (example mpi storage)
  add_executable       (${example}_actions  ${example}_actions.c)
  target_link_libraries(${example}_actions  simgrid)

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${example}_actions.tesh)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/${example}_actions.c)
  set(txt_files     ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/${example}_actions.txt)
  set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/${example}_deployment.xml)

  ADD_TESH_FACTORIES(msg-${example}-actions  "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/actions --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/actions ${example}_actions.tesh)
endforeach()

set(examples_src  ${examples_src}                                                          PARENT_SCOPE)
set(tesh_files    ${tesh_files}                                                            PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/mpi_deployment_split.xml    PARENT_SCOPE)
set(txt_files     ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/mpi_actions_split_p0.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/mpi_actions_split_p1.txt    PARENT_SCOPE)
