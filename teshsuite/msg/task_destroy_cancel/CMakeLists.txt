add_executable       (task_destroy_cancel task_destroy_cancel.c)
target_link_libraries(task_destroy_cancel simgrid)

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/task_destroy_cancel.tesh  PARENT_SCOPE)
set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/task_destroy_cancel.c     PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/task_destroy_cancel_d.xml PARENT_SCOPE)

ADD_TESH_FACTORIES(tesh-msg-task-destroy-cancel "thread;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/msg/task_destroy_cancel --cd ${CMAKE_BINARY_DIR}/teshsuite/msg/task_destroy_cancel ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/task_destroy_cancel/task_destroy_cancel.tesh)
