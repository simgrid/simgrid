add_executable       (pastry pastry.c)
target_link_libraries(pastry simgrid)

# ADD_TESH_FACTORIES(msg-pastry "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg/pastry --cd ${CMAKE_BINARY_DIR}/examples/msg/pastry ${CMAKE_HOME_DIRECTORY}/examples/msg/pastry/pastry.tesh)

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/pastry.tesh      PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/pastry10.xml     PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/pastry.c         PARENT_SCOPE)
