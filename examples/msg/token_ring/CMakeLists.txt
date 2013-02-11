cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(token_ring ring_call.c)
add_executable(token_bypass token_bypass.c)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(token_ring simgrid pthread )
  target_link_libraries(token_bypass simgrid pthread )
else()
  target_link_libraries(token_ring simgrid)
  target_link_libraries(token_bypass simgrid)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/token_ring.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/ring_call.c
  ${CMAKE_CURRENT_SOURCE_DIR}/token_bypass.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  PARENT_SCOPE
  )
