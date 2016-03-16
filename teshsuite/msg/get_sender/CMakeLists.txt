add_executable       (get_sender get_sender.c)
target_link_libraries(get_sender simgrid)

set(tesh_files     ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/get_sender.tesh  PARENT_SCOPE)
set(teshsuite_src  ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/get_sender.c     PARENT_SCOPE)

ADD_TESH_FACTORIES(tesh-msg-get-sender "thread;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/msg/get_sender --cd ${CMAKE_BINARY_DIR}/teshsuite/msg/get_sender ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/get_sender/get_sender.tesh)
