if(HAVE_GRAPHVIZ)
  add_executable       (dot_test     dot_test.c)
  target_link_libraries(dot_test     simgrid)
  add_executable       (ptg_test     ptg_test.c)
  target_link_libraries(ptg_test     simgrid)
  add_executable       (simulate_dot simulate_dot.c)
  target_link_libraries(simulate_dot simgrid)
  add_executable       (dot_test2    dot_test2.c)
  target_link_libraries(dot_test2    simgrid)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_simdag_dotload.tesh
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/dot_test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ptg_test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dot_test2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/simulate_dot.c
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/dag_with_bad_schedule.dot
  ${CMAKE_CURRENT_SOURCE_DIR}/dag_with_cycle.dot
  ${CMAKE_CURRENT_SOURCE_DIR}/dag_with_good_schedule.dot
  ${CMAKE_CURRENT_SOURCE_DIR}/dag.dot
  ${CMAKE_CURRENT_SOURCE_DIR}/ptg.dot
  PARENT_SCOPE)
