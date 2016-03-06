add_executable       (msg_prop msg_prop.c)
target_link_libraries(msg_prop simgrid)

ADD_TESH_FACTORIES(msg-properties "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/properties/msg_prop.tesh)

set(tesh_files     ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/msg_prop.tesh              PARENT_SCOPE)
set(xml_files      ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/deployment_properties.xml  PARENT_SCOPE)
set(examples_src   ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/msg_prop.c                 PARENT_SCOPE)
