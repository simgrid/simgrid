cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(log_usage "log_usage.c")
add_executable(graphxml_usage "graphxml_usage.c")
add_executable(heap_bench "heap_bench.c")
add_executable(parmap_bench "parmap_bench.c")

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(log_usage simgrid m )
  target_link_libraries(graphxml_usage simgrid m )
  target_link_libraries(heap_bench simgrid m )
  target_link_libraries(parmap_bench simgrid m )
else()
  target_link_libraries(log_usage simgrid )
  target_link_libraries(graphxml_usage simgrid )
  target_link_libraries(heap_bench simgrid )
  target_link_libraries(parmap_bench simgrid )
endif()

set(tesh_files
  ${tesh_files}
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/graph.xml
  PARENT_SCOPE
  )
set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/graphxml_usage.c
  ${CMAKE_CURRENT_SOURCE_DIR}/heap_bench.c
  ${CMAKE_CURRENT_SOURCE_DIR}/log_usage.c
  ${CMAKE_CURRENT_SOURCE_DIR}/parmap_bench.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  PARENT_SCOPE
  )
