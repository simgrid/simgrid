add_executable       (suspend suspend.c)
target_link_libraries(suspend simgrid)

ADD_TESH_FACTORIES(msg-suspend "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/suspend/suspend.tesh)

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/suspend.tesh  PARENT_SCOPE)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/suspend.c     PARENT_SCOPE)
