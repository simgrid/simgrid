foreach(x exception migration parallel_task pmm priority properties suspend synchro token_ring)
  add_executable       (${x}     ${x}/${x}.c)
  target_link_libraries(${x}     simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
  set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
endforeach()

foreach(file bourassa fafard ginette jupiter link3 link4 link5)
  set(txt_files ${txt_files} ${CMAKE_CURRENT_SOURCE_DIR}/${file}_state.trace)
endforeach()

set(txt_files    ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/README                       PARENT_SCOPE)
set(examples_src ${examples_src}                                                           PARENT_SCOPE)
set(tesh_files   ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/parallel_task/parallel_task_energy.tesh      PARENT_SCOPE)
set(xml_files    ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/priority/priority_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/properties/properties_d.xml                  PARENT_SCOPE)

foreach(x migration priority properties suspend synchro token_ring)
  ADD_TESH_FACTORIES(msg-${x} "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/${x} ${x}.tesh)
endforeach()

ADD_TESH_FACTORIES(msg-ptask        "thread;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/parallel_task/parallel_task.tesh)
ADD_TESH_FACTORIES(msg-ptask-energy "thread;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/parallel_task/parallel_task_energy.tesh)
ADD_TESH(msg-pmm  --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/pmm/pmm.tesh)
# This one is not usable:
# ADD_TESH_FACTORIES(msg-exception "thread;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/exception/exception.tesh)
