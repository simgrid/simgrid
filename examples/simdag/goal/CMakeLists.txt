set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(goal_test goal_test.c)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(goal_test simgrid pthread )
else()
  target_link_libraries(goal_test simgrid)
endif()

set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/goal_test.c
  PARENT_SCOPE)
