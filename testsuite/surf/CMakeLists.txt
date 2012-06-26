cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(lmm_usage "lmm_usage.c")
add_executable(maxmin_bench "maxmin_bench.c")
add_executable(surf_usage "surf_usage.c")
add_executable(surf_usage2 "surf_usage2.c")
add_executable(trace_usage "trace_usage.c")

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(lmm_usage simgrid m )
  target_link_libraries(maxmin_bench simgrid m )
  target_link_libraries(surf_usage simgrid m )
  target_link_libraries(surf_usage2 simgrid m )
  target_link_libraries(trace_usage simgrid m )
else(NOT WIN32)
  target_link_libraries(lmm_usage simgrid )
  target_link_libraries(maxmin_bench simgrid )
  set_target_properties(maxmin_bench PROPERTIES COMPILE_FLAGS "-DDLL_STATIC")
  target_link_libraries(surf_usage simgrid )
  target_link_libraries(surf_usage2 simgrid )
  target_link_libraries(trace_usage simgrid )
endif(NOT WIN32)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/simeng_usage.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/platform.xml
  PARENT_SCOPE
  )
set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/lmm_usage.c
  ${CMAKE_CURRENT_SOURCE_DIR}/maxmin_bench.c
  ${CMAKE_CURRENT_SOURCE_DIR}/surf_usage.c
  ${CMAKE_CURRENT_SOURCE_DIR}/surf_usage2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/trace_usage.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/trace_A_failure.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/trace_A.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/trace_B.txt
  PARENT_SCOPE
  )
