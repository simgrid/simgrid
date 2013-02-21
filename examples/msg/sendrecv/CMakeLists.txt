cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(sendrecv sendrecv.c)

### Add definitions for compile
target_link_libraries(sendrecv simgrid )

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv_CLM03.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv_KCCFLN05.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv_Reno.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv_Vegas.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_sendrecv.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_sendrecv.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/README
  PARENT_SCOPE
  )
