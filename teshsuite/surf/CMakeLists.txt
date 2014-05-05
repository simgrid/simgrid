cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/platform.xml
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/trace_A_failure.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/trace_A.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/trace_B.txt
  PARENT_SCOPE
  )
