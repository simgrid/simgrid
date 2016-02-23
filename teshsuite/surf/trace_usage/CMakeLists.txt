set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(trace_usage ${CMAKE_HOME_DIRECTORY}/teshsuite/surf/trace_usage/trace_usage.cpp)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(trace_usage simgrid m)
else()
  target_link_libraries(trace_usage simgrid )
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/trace_usage.tesh
  PARENT_SCOPE)
set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/trace_usage.cpp
  PARENT_SCOPE)
