if(enable_msg)
  add_executable       (task_destroy_cancel EXCLUDE_FROM_ALL task_destroy_cancel/task_destroy_cancel.c)
  target_link_libraries(task_destroy_cancel simgrid)
  set_target_properties(task_destroy_cancel PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/task_destroy_cancel)
  add_dependencies(tests task_destroy_cancel)

  ADD_TESH_FACTORIES(tesh-msg-task_destroy_cancel "raw"  --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                                                         --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/msg/task_destroy_cancel
                                                         --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/task_destroy_cancel 
                                                         ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/task_destroy_cancel/task_destroy_cancel.tesh)
endif()

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/task_destroy_cancel/task_destroy_cancel.tesh PARENT_SCOPE)
set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/task_destroy_cancel/task_destroy_cancel.c PARENT_SCOPE)
