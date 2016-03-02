add_executable       (synchro synchro.c)
target_link_libraries(synchro simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/synchro.tesh
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/synchro.c
  PARENT_SCOPE)
