foreach(x daxload typed_tasks scheduling)
  add_executable       (sd_${x}  EXCLUDE_FROM_ALL  ${x}/sd_${x}.c)
  target_link_libraries(sd_${x}     simgrid)
  set_target_properties(sd_${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  add_dependencies(tests sd_${x})
  set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/sd_${x}.c)
  set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${x}/sd_${x}.tesh)
endforeach()

foreach(x test)
  add_executable       (sd_${x}  EXCLUDE_FROM_ALL ${x}/sd_${x}.cpp)
  target_link_libraries(sd_${x}     simgrid)
  set_target_properties(sd_${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  add_dependencies(tests sd_${x})
  set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/sd_${x}.cpp)
  set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${x}/sd_${x}.tesh)
endforeach()

foreach(x dag-dotload schedule-dotload)
  if(HAVE_GRAPHVIZ)
    add_executable       (sd_${x}  EXCLUDE_FROM_ALL ${x}/sd_${x}.c)
    target_link_libraries(sd_${x}     simgrid)
    set_target_properties(sd_${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
    add_dependencies(tests sd_${x})
  endif()
  set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/sd_${x}.c)
  set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${x}/sd_${x}.tesh)
endforeach()

set(tesh_files   ${tesh_files}                                                                             PARENT_SCOPE)
set(examples_src ${examples_src}                                                                           PARENT_SCOPE)
set(xml_files    ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/scheduling/Montage_25.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/daxload/simple_dax_with_cycle.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/daxload/smalldax.xml                         PARENT_SCOPE)
set(txt_files    ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/dag-dotload/dag_with_cycle.dot
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dag-dotload/dag.dot
                                  ${CMAKE_CURRENT_SOURCE_DIR}/schedule-dotload/dag_with_bad_schedule.dot
                                  ${CMAKE_CURRENT_SOURCE_DIR}/schedule-dotload/dag_with_good_schedule.dot
                                  ${CMAKE_CURRENT_SOURCE_DIR}/scheduling/expected_output.jed               PARENT_SCOPE)

foreach(x daxload typed_tasks scheduling test)
  ADD_TESH(simdag-${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/deprecated/simdag --cd ${CMAKE_BINARY_DIR}/examples/deprecated/simdag ${CMAKE_HOME_DIRECTORY}/examples/deprecated/simdag/${x}/sd_${x}.tesh)
endforeach()

if(HAVE_GRAPHVIZ)
  foreach(x dag-dotload schedule-dotload)
    ADD_TESH(simdag-${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/deprecated/simdag --cd ${CMAKE_BINARY_DIR}/examples/deprecated/simdag ${CMAKE_HOME_DIRECTORY}/examples/deprecated/simdag/${x}/sd_${x}.tesh)
  endforeach()
endif()
