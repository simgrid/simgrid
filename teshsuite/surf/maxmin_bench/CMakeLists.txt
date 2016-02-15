set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(maxmin_bench ${CMAKE_HOME_DIRECTORY}/teshsuite/surf/maxmin_bench/maxmin_bench.c)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(maxmin_bench simgrid m )
else()
  target_link_libraries(maxmin_bench simgrid )
  set_target_properties(maxmin_bench PROPERTIES COMPILE_FLAGS "-DDLL_STATIC")
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/maxmin_bench.tesh
  PARENT_SCOPE)
set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/maxmin_bench.c
  PARENT_SCOPE)
