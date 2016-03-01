add_executable       (host_on_off_processes host_on_off_processes.c)
target_link_libraries(host_on_off_processes simgrid)

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off_processes.tesh   PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off_processes_d.xml  PARENT_SCOPE)
set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off_processes.c      PARENT_SCOPE)

ADD_TESH_FACTORIES(tesh-msg-host-on-off-processes "thread;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/msg/host_on_off_processes --cd ${CMAKE_BINARY_DIR}/teshsuite/msg/host_on_off_processes ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/host_on_off_processes/host_on_off_processes.tesh)
