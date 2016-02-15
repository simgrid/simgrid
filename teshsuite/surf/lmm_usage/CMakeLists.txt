set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(lmm_usage ${CMAKE_HOME_DIRECTORY}/teshsuite/surf/lmm_usage/lmm_usage.c)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(lmm_usage simgrid m )
else()
  target_link_libraries(lmm_usage simgrid )
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/lmm_usage.tesh
  PARENT_SCOPE)
set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/lmm_usage.c
  PARENT_SCOPE)
