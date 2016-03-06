foreach (example file file_unlink storage remote)
  add_executable       (${example}  ${example}.c)
  target_link_libraries(${example}  simgrid)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/${example}.c)
  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${example}.tesh)

  ADD_TESH(msg-${example} --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/ --setenv srcdir=${CMAKE_HOME_DIRECTORY}/ --cd ${CMAKE_HOME_DIRECTORY}/examples/ ${CMAKE_HOME_DIRECTORY}/examples/msg/io/${example}.tesh)
endforeach()

set(examples_src  ${examples_src}                                           PARENT_SCOPE)
set(tesh_files    ${tesh_files}                                             PARENT_SCOPE)
set(xml_files     ${xml_files}    examples/msg/io/remote_io.deployment.xml  PARENT_SCOPE)
