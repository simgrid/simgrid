set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(availability_test availability_test.c)
target_link_libraries(availability_test simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/availability_test.tesh
  PARENT_SCOPE)
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/availability_test.c
  PARENT_SCOPE)
