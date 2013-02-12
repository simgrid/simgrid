cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(test_trace_integration test_trace_integration.c)

### Add definitions for compile
target_link_libraries(test_trace_integration simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/trace.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/test10.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/test11.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/test1.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/test2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/test3.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/test4.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/test5.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/test6.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/test7.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/test8.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/test9.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/test_trace_integration.xml
  PARENT_SCOPE
  )
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_trace_integration.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/file.trace
  PARENT_SCOPE
  )
