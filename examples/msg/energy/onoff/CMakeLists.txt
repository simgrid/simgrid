cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(onoff onoff.c)

### Add definitions for compile
target_link_libraries(onoff simgrid )

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/energy_consumption.tesh
  PARENT_SCOPE
  )

set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_onoff.xml
  PARENT_SCOPE
  )

set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/onoff.c
  PARENT_SCOPE
  )

set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
