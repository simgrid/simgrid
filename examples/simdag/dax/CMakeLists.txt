set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(dax_test dax_test.c)
target_link_libraries(dax_test simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/simple_dax_with_cycle.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/smalldax.tesh
  PARENT_SCOPE)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/simple_dax_with_cycle.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/smalldax.xml
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/dax_test.c
  PARENT_SCOPE)
