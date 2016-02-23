add_executable       (token_ring ring_call.c)
target_link_libraries(token_ring simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/token_ring.tesh
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/ring_call.c
  PARENT_SCOPE)
