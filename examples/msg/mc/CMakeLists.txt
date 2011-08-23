cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(centralized centralized_mutex.c)
add_executable(bugged1     bugged1.c)
add_executable(bugged1_stateful bugged1_stateful.c)
add_executable(bugged2_stateful bugged2_stateful.c)
add_executable(bugged2     bugged2.c)
add_executable(bugged3	   bugged3.c)
add_executable(random_test random_test.c)
add_executable(example_liveness_without_cycle automaton.c
automatonparse_promela.c example_liveness_without_cycle.c)
add_executable(example2_liveness_without_cycle automaton.c
automatonparse_promela.c
example2_liveness_without_cycle.c)


target_link_libraries(centralized simgrid m )
target_link_libraries(bugged1     simgrid m )
target_link_libraries(bugged1_stateful simgrid m)
target_link_libraries(bugged2_stateful simgrid m)
target_link_libraries(bugged2     simgrid m )
target_link_libraries(bugged3     simgrid m )
target_link_libraries(random_test     simgrid m )
target_link_libraries(example_liveness_without_cycle     simgrid m )
target_link_libraries(example2_liveness_without_cycle     simgrid m )