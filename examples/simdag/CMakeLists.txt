cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(ex_sd_test sd_test.c)
add_executable(sd_test2 sd_test2.c)
add_executable(sd_fail sd_fail.c)
add_executable(sd_avail sd_avail.c)
add_executable(sd_typed_tasks_test sd_typed_tasks_test.c)
add_executable(sd_comm_throttling sd_comm_throttling.c)
add_executable(sd_seq_access sd_seq_access.c)
add_executable(simdag_tracing simdag_trace.c)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(ex_sd_test simgrid pthread m )
  target_link_libraries(sd_test2 simgrid pthread m )
  target_link_libraries(sd_fail simgrid pthread m )
  target_link_libraries(sd_avail simgrid pthread m )
  target_link_libraries(sd_typed_tasks_test simgrid pthread m )
  target_link_libraries(sd_comm_throttling simgrid pthread m )
  target_link_libraries(sd_seq_access simgrid pthread m )
  target_link_libraries(simdag_tracing simgrid pthread m )

  add_custom_command(TARGET ex_sd_test
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/ex_sd_test ${CMAKE_CURRENT_BINARY_DIR}/sd_test
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    )
  get_directory_property(extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
  set_directory_properties(
    PROPERTIES
    ADDITIONAL_MAKE_CLEAN_FILES "${extra_clean_files};${CMAKE_CURRENT_BINARY_DIR}/sd_test")

else()
  target_link_libraries(ex_sd_test simgrid)
  target_link_libraries(sd_test2 simgrid)
  target_link_libraries(sd_fail simgrid)
  target_link_libraries(sd_avail simgrid)
  target_link_libraries(sd_typed_tasks_test simgrid)
  target_link_libraries(sd_comm_throttling simgrid)
  target_link_libraries(sd_seq_access simgrid)
  target_link_libraries(simdag_tracing simgrid)

  add_custom_command(TARGET ex_sd_test
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/ex_sd_test.exe ${CMAKE_CURRENT_BINARY_DIR}/sd_test.exe
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    )
  get_directory_property(extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
  set_directory_properties(
    PROPERTIES
    ADDITIONAL_MAKE_CLEAN_FILES "${extra_clean_files};${CMAKE_CURRENT_BINARY_DIR}/sd_test.exe")
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_simdag.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_simdag2.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_simdag2_lua.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_simdag_comm_throttling.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_simdag_seq_access.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_simdag_tracing.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_simdag_typed_tasks.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_simdag_fail.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/2clusters.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/two_hosts.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/faulty_host.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/sd_avail.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sd_comm_throttling.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sd_fail.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sd_seq_access.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sd_test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sd_test2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sd_typed_tasks_test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/simdag_trace.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_script.lua
  ${CMAKE_CURRENT_SOURCE_DIR}/test_simdag.in
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/faulty_host.trace
  ${CMAKE_CURRENT_SOURCE_DIR}/jupiter_power.trace
  ${CMAKE_CURRENT_SOURCE_DIR}/link1_avail.trace
  PARENT_SCOPE
  )
