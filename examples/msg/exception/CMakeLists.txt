add_executable       (exception exception.c)
target_link_libraries(exception simgrid)

# This one is not usable:
# ADD_TESH_FACTORIES(msg-exception "thread;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/exception/exception.tesh)

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/exception.tesh            PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/exception.c               PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/deployment_exception.xml  PARENT_SCOPE)
