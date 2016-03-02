add_executable       (listen_async listen_async.c)
target_link_libraries(listen_async simgrid)

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/listen_async.tesh  PARENT_SCOPE)
set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/listen_async.c     PARENT_SCOPE)

ADD_TESH_FACTORIES(tesh-msg-listen-async "thread;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/msg/listen_async --cd ${CMAKE_BINARY_DIR}/teshsuite/msg/listen_async/ ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/listen_async/listen_async.tesh)
