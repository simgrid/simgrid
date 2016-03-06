add_executable       (msg_pmm msg_pmm.c)
target_link_libraries(msg_pmm simgrid)

ADD_TESH(msg-pmm  --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/pmm/pmm.tesh)

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/pmm.tesh   PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/msg_pmm.c  PARENT_SCOPE)
