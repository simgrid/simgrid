set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(token_ring ring_call.c)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(token_ring simgrid pthread )
else()
  target_link_libraries(token_ring simgrid)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/token_ring.tesh
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/ring_call.c
  PARENT_SCOPE)
