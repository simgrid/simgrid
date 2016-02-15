set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(sd_prop sd_prop.c)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(sd_prop simgrid pthread )
else()
  target_link_libraries(sd_prop simgrid)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_prop.tesh
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/sd_prop.c
  PARENT_SCOPE)
