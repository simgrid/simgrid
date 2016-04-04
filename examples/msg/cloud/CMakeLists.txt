foreach (example simple_vm scale two_tasks_vm)
  add_executable       (${example} ${example}.c)
  target_link_libraries(${example} simgrid)
  set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${example}.c)
endforeach()

ADD_TESH_FACTORIES(msg-cloud-two-tasks-vm      "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/cloud/ --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg/cloud/ --cd ${CMAKE_BINARY_DIR}/examples/msg/cloud/ ${CMAKE_HOME_DIRECTORY}/examples/msg/cloud/two_tasks_vm.tesh)
ADD_TESH_FACTORIES(msg-cloud-simple-vm         "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/cloud/ --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg/cloud/ --cd ${CMAKE_BINARY_DIR}/examples/msg/cloud/ ${CMAKE_HOME_DIRECTORY}/examples/msg/cloud/simple_vm.tesh)

set(examples_src  ${examples_src}                                                    PARENT_SCOPE)
set(tesh_files    ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/two_tasks_vm.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/simple_vm.tesh         PARENT_SCOPE)
