cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(e1 e1.c)

### Add definitions for compile
target_link_libraries(e1 simgrid )

set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_e1.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_e1.xml
  PARENT_SCOPE
  )

set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
