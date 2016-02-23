add_executable       (lmm_usage lmm_usage.c)
target_link_libraries(lmm_usage simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/lmm_usage.tesh
  PARENT_SCOPE)
set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/lmm_usage.c
  PARENT_SCOPE)
