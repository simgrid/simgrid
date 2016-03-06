add_executable       (sk_time sk_time.c)
target_link_libraries(sk_time simgrid)

ADD_TESH(msg-start-kill-time --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/ --setenv srcdir=${CMAKE_HOME_DIRECTORY}/ --cd ${CMAKE_HOME_DIRECTORY}/examples/ ${CMAKE_HOME_DIRECTORY}/examples/msg/start_kill_time/start_kill_time.tesh)

set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/sk_time.c             PARENT_SCOPE)
set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/start_kill_time.tesh  PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/deployment_kill.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/deployment_start_kill.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/deployment_start.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/deployment.xml        PARENT_SCOPE)
