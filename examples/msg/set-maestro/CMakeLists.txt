add_executable       (set-maestro set-maestro.c)
target_link_libraries(set-maestro simgrid)

ADD_TESH_FACTORIES(msg-set-maestro "thread" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/set-maestro/set-maestro.tesh)

set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/set-maestro.tesh  PARENT_SCOPE)
set(xml_files    ${xml_files}    ${CMAKE_CURRENT_SOURCE_DIR}/set-maestro_d.xml PARENT_SCOPE)
set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/set-maestro.c     PARENT_SCOPE)
