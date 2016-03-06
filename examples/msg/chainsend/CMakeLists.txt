add_executable       (chainsend chainsend.c iterator.c common.c messages.c broadcaster.c peer.c)
target_link_libraries(chainsend simgrid)

ADD_TESH(msg-chainsend --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/chainsend --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg/chainsend --cd ${CMAKE_HOME_DIRECTORY}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/chainsend/chainsend.tesh)

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/chainsend.tesh       PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/deployment_tesh.xml  PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/iterator.h
                                   ${CMAKE_CURRENT_SOURCE_DIR}/iterator.c
                                   ${CMAKE_CURRENT_SOURCE_DIR}/common.h
                                   ${CMAKE_CURRENT_SOURCE_DIR}/common.c
                                   ${CMAKE_CURRENT_SOURCE_DIR}/messages.h
                                   ${CMAKE_CURRENT_SOURCE_DIR}/messages.c
                                   ${CMAKE_CURRENT_SOURCE_DIR}/broadcaster.h
                                   ${CMAKE_CURRENT_SOURCE_DIR}/broadcaster.c
                                   ${CMAKE_CURRENT_SOURCE_DIR}/peer.h
                                   ${CMAKE_CURRENT_SOURCE_DIR}/peer.c
                                   ${CMAKE_CURRENT_SOURCE_DIR}/chainsend.c          PARENT_SCOPE)
