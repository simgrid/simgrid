add_executable       (process_join process_join.c)
target_link_libraries(process_join simgrid)

set(tesh_files    ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/process_join.tesh   PARENT_SCOPE)
set(teshsuite_src ${teshsuite_src}  ${CMAKE_CURRENT_SOURCE_DIR}/process_join.c      PARENT_SCOPE)

ADD_TESH_FACTORIES(tesh-msg-process-join "thread;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/msg/process_join --cd ${CMAKE_BINARY_DIR}/teshsuite/msg/process_join ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/process_join/process_join.tesh)
