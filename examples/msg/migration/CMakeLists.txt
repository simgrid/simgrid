add_executable       (migration migration.c)
target_link_libraries(migration simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/migration.tesh
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/migration.c
  PARENT_SCOPE)
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/migration.deploy
  PARENT_SCOPE)
