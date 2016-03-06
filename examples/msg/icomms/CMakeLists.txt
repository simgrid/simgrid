foreach (example peer peer2 peer3)
  add_executable       (${example}  ${example}.c)
  target_link_libraries(${example}  simgrid)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/${example}.c)
endforeach()

ADD_TESH_FACTORIES(msg-icomms  "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg/icomms --cd ${CMAKE_BINARY_DIR}/examples/msg/icomms ${CMAKE_HOME_DIRECTORY}/examples/msg/icomms/peer.tesh)
ADD_TEST(msg-icomms-waitany    ${CMAKE_BINARY_DIR}/examples/msg/icomms/peer3 ${CMAKE_HOME_DIRECTORY}/examples/platforms/small_platform.xml ${CMAKE_HOME_DIRECTORY}/examples/msg/icomms/deployment_peer05.xml)

set(examples_src  ${examples_src}                                                     PARENT_SCOPE)
set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/peer.tesh              PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/deployment_peer.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/deployment_peer01.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/deployment_peer02.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/deployment_peer03.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/deployment_peer04.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/deployment_peer05.xml  PARENT_SCOPE)
