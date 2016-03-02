add_executable       (sd_test             sd_test.c)
target_link_libraries(sd_test             simgrid)
add_executable       (sd_test2            sd_test2.c)
target_link_libraries(sd_test2            simgrid)
add_executable       (sd_fail             sd_fail.c)
target_link_libraries(sd_fail             simgrid)
add_executable       (sd_avail            sd_avail.c)
target_link_libraries(sd_avail            simgrid)
add_executable       (sd_typed_tasks_test sd_typed_tasks_test.c)
target_link_libraries(sd_typed_tasks_test simgrid)
add_executable       (sd_comm_throttling  sd_comm_throttling.c)
target_link_libraries(sd_comm_throttling  simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_simdag.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_simdag2.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_simdag_comm_throttling.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_simdag_typed_tasks.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_simdag_avail.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_simdag_fail.tesh
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/sd_avail.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sd_comm_throttling.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sd_fail.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sd_test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sd_test2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sd_typed_tasks_test.c
  PARENT_SCOPE)
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_script.lua
  PARENT_SCOPE)
