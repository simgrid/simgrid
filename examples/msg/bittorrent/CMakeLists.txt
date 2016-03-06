add_executable       (bittorrent bittorrent.c messages.c peer.c tracker.c connection.c)
target_link_libraries(bittorrent simgrid)

ADD_TESH_FACTORIES(msg-bittorrent              "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/bittorrent --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/bittorrent bittorrent.tesh)
ADD_TESH_FACTORIES(msg-bittorrent-parallel     "thread;ucontext;raw" --cfg contexts/nthreads:4  ${CONTEXTS_SYNCHRO} --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/bittorrent --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/bittorrent bittorrent.tesh)

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/bittorrent.tesh    PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/bittorrent.xml     PARENT_SCOPE)
set(bin_files     ${bin_files}     ${CMAKE_CURRENT_SOURCE_DIR}/generate.py        PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/bittorrent.c
                                   ${CMAKE_CURRENT_SOURCE_DIR}/bittorrent.h
                                   ${CMAKE_CURRENT_SOURCE_DIR}/connection.c
                                   ${CMAKE_CURRENT_SOURCE_DIR}/connection.h
                                   ${CMAKE_CURRENT_SOURCE_DIR}/messages.c
                                   ${CMAKE_CURRENT_SOURCE_DIR}/messages.h
                                   ${CMAKE_CURRENT_SOURCE_DIR}/peer.c
                                   ${CMAKE_CURRENT_SOURCE_DIR}/peer.h
                                   ${CMAKE_CURRENT_SOURCE_DIR}/tracker.c
                                   ${CMAKE_CURRENT_SOURCE_DIR}/tracker.h         PARENT_SCOPE)
