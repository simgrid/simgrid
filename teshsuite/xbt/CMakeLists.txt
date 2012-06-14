cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(log_large_test log_large_test.c)
add_executable(parallel_log_crashtest parallel_log_crashtest.c)

### Add definitions for compile
if(NOT WIN32)
target_link_libraries(log_large_test gras m pthread )
target_link_libraries(parallel_log_crashtest gras m pthread )
else(NOT WIN32)
target_link_libraries(log_large_test gras)
target_link_libraries(parallel_log_crashtest gras)
endif(NOT WIN32)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/log_large_test.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/parallel_log_crashtest.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/xbt.tesh
  PARENT_SCOPE
)
set(xml_files
  ${xml_files}
  PARENT_SCOPE
)
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/log_large_test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/parallel_log_crashtest.c
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