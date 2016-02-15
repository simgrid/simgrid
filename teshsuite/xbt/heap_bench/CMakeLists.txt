set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(heap_bench ${CMAKE_HOME_DIRECTORY}/teshsuite/xbt/heap_bench/heap_bench.c)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(heap_bench simgrid m )
else()
  target_link_libraries(heap_bench simgrid )
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/heap_bench.tesh
  PARENT_SCOPE)
set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/heap_bench.c
  PARENT_SCOPE)
