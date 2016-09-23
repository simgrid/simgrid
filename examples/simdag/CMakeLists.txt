foreach(x availability daxload fail typed_tasks properties throttling scheduling)
  add_executable       (sd_${x}     ${x}/sd_${x}.c)
  target_link_libraries(sd_${x}     simgrid)
  set_target_properties(sd_${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/sd_${x}.c)
  set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${x}/sd_${x}.tesh)
endforeach()

foreach(x test)
  add_executable       (sd_${x}     ${x}/sd_${x}.cpp)
  target_link_libraries(sd_${x}     simgrid)
  set_target_properties(sd_${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/sd_${x}.cpp)
  set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${x}/sd_${x}.tesh)
endforeach()

foreach(x dag-dotload ptg-dotload schedule-dotload)
  if(HAVE_GRAPHVIZ)
    add_executable       (sd_${x}     ${x}/sd_${x}.c)
    target_link_libraries(sd_${x}     simgrid)
    set_target_properties(sd_${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  endif()
  set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/sd_${x}.c)
  set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${x}/sd_${x}.tesh)
endforeach()

add_executable       (goal_test goal/goal_test.c)
target_link_libraries(goal_test simgrid)
set_target_properties(goal_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/goal)

set(tesh_files   ${tesh_files}                                                                             PARENT_SCOPE)
set(examples_src ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/goal/goal_test.c                             PARENT_SCOPE)
set(xml_files    ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/scheduling/Montage_25.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/daxload/simple_dax_with_cycle.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/daxload/smalldax.xml                         PARENT_SCOPE)
set(txt_files    ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/dag-dotload/dag_with_cycle.dot
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dag-dotload/dag.dot
                                  ${CMAKE_CURRENT_SOURCE_DIR}/ptg-dotload/ptg.dot
                                  ${CMAKE_CURRENT_SOURCE_DIR}/schedule-dotload/dag_with_bad_schedule.dot
                                  ${CMAKE_CURRENT_SOURCE_DIR}/schedule-dotload/dag_with_good_schedule.dot
                                  ${CMAKE_CURRENT_SOURCE_DIR}/scheduling/expected_output.jed               PARENT_SCOPE)

foreach(x availability daxload fail typed_tasks properties throttling scheduling test)
  ADD_TESH(simdag-${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/simdag --cd ${CMAKE_BINARY_DIR}/examples/simdag ${CMAKE_HOME_DIRECTORY}/examples/simdag/${x}/sd_${x}.tesh)
endforeach()

if(HAVE_GRAPHVIZ)
  foreach(x dag-dotload ptg-dotload schedule-dotload)
    ADD_TESH(simdag-${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/simdag --cd ${CMAKE_BINARY_DIR}/examples/simdag ${CMAKE_HOME_DIRECTORY}/examples/simdag/${x}/sd_${x}.tesh)
  endforeach()
endif()
