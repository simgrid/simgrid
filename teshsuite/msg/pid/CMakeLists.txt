add_executable       (pid pid.c)
target_link_libraries(pid simgrid)

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/pid.tesh   PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/pid_d.xml  PARENT_SCOPE)
set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/pid.c      PARENT_SCOPE)

ADD_TESH_FACTORIES(tesh-msg-pid "thread;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/msg/pid --cd ${CMAKE_BINARY_DIR}/teshsuite/msg/pid ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/pid/pid.tesh)
