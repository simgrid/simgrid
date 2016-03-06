foreach (example consumption onoff pstate vm)
  add_executable       (${example}  ${example}/${example}.c)
  target_link_libraries(${example}  simgrid)
  set_target_properties(${example}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${example})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.tesh)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/${example}/${example}.c)
  ADD_TESH_FACTORIES(msg-energy-${example} "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/energy/${example}/${example}.tesh)
endforeach()

ADD_TESH_FACTORIES(msg-ptask-energy-pstates     "thread;ucontext;raw;boost" --cfg host/model:ptask_L07 --log xbt_cfg.threshold:critical --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/energy/pstate/pstate.tesh)
ADD_TESH_FACTORIES(msg-ptask-energy-consumption "thread;ucontext;raw;boost" --cfg host/model:ptask_L07 --log xbt_cfg.threshold:critical --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/energy/consumption/consumption.tesh)

set(examples_src  ${examples_src}                                                                        PARENT_SCOPE)
set(tesh_files    ${tesh_files}                                                                          PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/onoff/platform_onoff.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/consumption/deployment_consumption.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/onoff/deployment_onoff.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/pstate/deployment_pstate.xml              PARENT_SCOPE)
