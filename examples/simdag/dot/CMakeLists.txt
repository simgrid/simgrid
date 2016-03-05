if(HAVE_GRAPHVIZ)
  foreach(x dot_test ptg_test simulate_dot dot_test2)
    add_executable       (${x}     ${x}.c)
    target_link_libraries(${x}     simgrid)
    set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}.c)
  endforeach()

  ADD_TESH(simdag-dotload --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/simdag/dot --cd ${CMAKE_BINARY_DIR}/examples/simdag/dot ${CMAKE_HOME_DIRECTORY}/examples/simdag/dot/test_simdag_dotload.tesh)
endif()

set(examples_src ${examples_src}                                                          PARENT_SCOPE)
set(tesh_files   ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/test_simdag_dotload.tesh    PARENT_SCOPE)
set(txt_files    ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/dag_with_bad_schedule.dot
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dag_with_cycle.dot
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dag_with_good_schedule.dot
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dag.dot
                                  ${CMAKE_CURRENT_SOURCE_DIR}/ptg.dot                     PARENT_SCOPE)
