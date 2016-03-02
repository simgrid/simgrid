add_executable(trace_usage trace_usage.cpp)
target_link_libraries(trace_usage simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/trace_usage.tesh
  PARENT_SCOPE)
set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/trace_usage.cpp
  PARENT_SCOPE)
