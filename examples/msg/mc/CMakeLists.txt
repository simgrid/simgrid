cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(centralized centralized_mutex.c)
add_executable(bugged1     bugged1.c)
add_executable(bugged1_stateful bugged1_stateful.c)
add_executable(bugged2_stateful bugged2_stateful.c)
add_executable(bugged2     bugged2.c)
add_executable(bugged3	   bugged3.c)
add_executable(random_test random_test.c)
add_executable(bugged1_for_liveness automaton.c automatonparse_promela.c bugged1_for_liveness.c)
add_executable(bugged1_while_liveness automaton.c automatonparse_promela.c bugged1_while_liveness.c)
add_executable(bugged2_liveness automaton.c automatonparse_promela.c bugged2_liveness.c)
add_executable(centralized_liveness automaton.c automatonparse_promela.c centralized_liveness.c)
add_executable(centralized_liveness_deadlock automaton.c automatonparse_promela.c centralized_liveness_deadlock.c)
add_executable(test_snapshot automaton.c automatonparse_promela.c test_snapshot.c)

target_link_libraries(centralized simgrid m )
target_link_libraries(bugged1     simgrid m )
target_link_libraries(bugged1_stateful simgrid m)
target_link_libraries(bugged2_stateful simgrid m)
target_link_libraries(bugged2     simgrid m )
target_link_libraries(bugged3     simgrid m )
target_link_libraries(random_test     simgrid m )
target_link_libraries(bugged1_for_liveness     simgrid m )
target_link_libraries(bugged1_while_liveness     simgrid m )
target_link_libraries(bugged2_liveness     simgrid m )
target_link_libraries(centralized_liveness     simgrid m )
target_link_libraries(centralized_liveness_deadlock     simgrid m )
target_link_libraries(test_snapshot     simgrid m )

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged2.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/centralized.tesh
  PARENT_SCOPE
)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deploy_bugged1_liveness.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deploy_bugged1.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deploy_bugged2_liveness.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deploy_bugged2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deploy_bugged3.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deploy_cennralized_liveness.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deploy_mutex.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deploy_random_test.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/platform.xml
  PARENT_SCOPE
)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/automanon.c
  ${CMAKE_CURRENT_SOURCE_DIR}/automatonparse_promela.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1_for_liveness.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1_stateful.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1_while_liveness.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged2_liveness.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged2_stateful.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged3.c
  ${CMAKE_CURRENT_SOURCE_DIR}/centralized_liveness.c
  ${CMAKE_CURRENT_SOURCE_DIR}/centralized_liveness_deadlock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/centralized_munex.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lex.yy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/random_test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test_snapshot.c
  ${CMAKE_CURRENT_SOURCE_DIR}/y.tab.c
  ${CMAKE_CURRENT_SOURCE_DIR}/automaton.h
  ${CMAKE_CURRENT_SOURCE_DIR}/automatonparse_promela.h
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged1_liveness.h
  ${CMAKE_CURRENT_SOURCE_DIR}/bugged2_liveness.h
  ${CMAKE_CURRENT_SOURCE_DIR}/centralized_liveness.h
  ${CMAKE_CURRENT_SOURCE_DIR}/test_snapshot.h
  ${CMAKE_CURRENT_SOURCE_DIR}/y.tab.h
  PARENT_SCOPE
)
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/dwarf
  ${CMAKE_CURRENT_SOURCE_DIR}/parse_dwarf
  ${CMAKE_CURRENT_SOURCE_DIR}/parserPromela.lex
  ${CMAKE_CURRENT_SOURCE_DIR}/parserPromela.yacc
  ${CMAKE_CURRENT_SOURCE_DIR}/promela1_bugged1_liveness
  ${CMAKE_CURRENT_SOURCE_DIR}/promela2_bugged1_liveness
  ${CMAKE_CURRENT_SOURCE_DIR}/promela2_bugged2_liveness
  ${CMAKE_CURRENT_SOURCE_DIR}/promela2_centralized_liveness
  ${CMAKE_CURRENT_SOURCE_DIR}/promela_bugged2_liveness
  ${CMAKE_CURRENT_SOURCE_DIR}/promela_centralized_liveness
  
  PARENT_SCOPE
)
set(txt_files
  ${txt_files}
  PARENT_SCOPE
)
