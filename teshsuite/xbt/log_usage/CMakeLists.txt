cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(log_usage ${CMAKE_HOME_DIRECTORY}/teshsuite/xbt/log_usage/log_usage.c)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(log_usage simgrid m )
else()
  target_link_libraries(log_usage simgrid )
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/log_usage.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/log_usage_ndebug.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  PARENT_SCOPE
  )
set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/log_usage.c
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
