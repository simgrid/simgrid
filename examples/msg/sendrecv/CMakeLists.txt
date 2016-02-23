set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(sendrecv sendrecv.c)
add_executable(sendrecv_main sendrecv_main.c)

### Add definitions for compile
target_link_libraries(sendrecv simgrid )
target_link_libraries(sendrecv_main simgrid )

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv_CLM03_main.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv_CLM03.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv_KCCFLN05.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv_Reno.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv_Vegas.tesh
  PARENT_SCOPE)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_sendrecv.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_sendrecv_main.xml
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv_main.c
  PARENT_SCOPE)
set(bin_files
  ${bin_files}
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/README
  PARENT_SCOPE)
