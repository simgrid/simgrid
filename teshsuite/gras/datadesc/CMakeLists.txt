cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(datadesc_usage datadesc_usage.c datadesc_structs.c)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(datadesc_usage gras m pthread)
elseif(NOT WIN32)
  target_link_libraries(datadesc_usage gras)
endif(NOT WIN32)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/datadesc_r_little32_4.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/datadesc_r_big32_8_4.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/datadesc_mem.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/datadesc_r_little64.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/datadesc.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/datadesc_rw.tesh
  PARENT_SCOPE
)
set(xml_files
  ${xml_files}
  PARENT_SCOPE
)
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/datadesc_usage.c
  ${CMAKE_CURRENT_SOURCE_DIR}/datadesc_structs.c
  PARENT_SCOPE
)
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/mk_datadesc_structs.pl
  PARENT_SCOPE
)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/datadesc.big32_8_4
  ${CMAKE_CURRENT_SOURCE_DIR}/datadesc.big64
  ${CMAKE_CURRENT_SOURCE_DIR}/datadesc.little32_4
  ${CMAKE_CURRENT_SOURCE_DIR}/datadesc.little64
  PARENT_SCOPE
)