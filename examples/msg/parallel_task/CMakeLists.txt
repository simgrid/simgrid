add_executable       (parallel_task parallel_task.c)
target_link_libraries(parallel_task simgrid)

ADD_TESH_FACTORIES(msg-ptask "thread;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/parallel_task/parallel_task.tesh)

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/parallel_task.tesh  PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/parallel_task.c     PARENT_SCOPE)
