add_executable       (goal_test goal_test.c)
target_link_libraries(goal_test simgrid)
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/goal_test.c  PARENT_SCOPE)
