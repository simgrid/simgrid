add_executable       (check_defaults check_defaults.c)
target_link_libraries(check_defaults simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/factory_raw.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/factory_thread.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/factory_ucontext.tesh
  PARENT_SCOPE)
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/check_defaults.c
  PARENT_SCOPE)
