add_executable       (synchro synchro.c)
target_link_libraries(synchro simgrid)

ADD_TESH_FACTORIES(msg-semaphores  "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/semaphores/synchro.tesh)

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/synchro.tesh  PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/synchro.c     PARENT_SCOPE)
