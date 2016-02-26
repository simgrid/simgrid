add_executable       (suspend suspend.c)
target_link_libraries(suspend simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/suspend.tesh
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/suspend.c
  PARENT_SCOPE)
