cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(e2 e2.c)

### Add definitions for compile
target_link_libraries(e2 simgrid )

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/energy_consumption.tesh
  PARENT_SCOPE
  )

set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_e2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_e2.xml
  PARENT_SCOPE
  )

set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/e2.c
  PARENT_SCOPE
  )

set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
