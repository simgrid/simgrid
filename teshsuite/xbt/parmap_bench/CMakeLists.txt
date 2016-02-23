set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(parmap_bench ${CMAKE_HOME_DIRECTORY}/teshsuite/xbt/parmap_bench/parmap_bench.c)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(parmap_bench simgrid m )
else()
  target_link_libraries(parmap_bench simgrid )
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/parmap_bench.tesh
  PARENT_SCOPE)
set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/parmap_bench.c
  PARENT_SCOPE)
