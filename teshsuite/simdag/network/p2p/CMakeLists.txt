cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(test_latency1 test_latency1.c)
add_executable(test_latency2 test_latency2.c)
add_executable(test_latency3 test_latency3.c)
add_executable(test_latency_bound test_latency_bound.c)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(test_latency1 simgrid m pthread )
  target_link_libraries(test_latency2 simgrid m pthread )
  target_link_libraries(test_latency3 simgrid m pthread )
  target_link_libraries(test_latency_bound simgrid m pthread )
else()
  target_link_libraries(test_latency1 simgrid)
  target_link_libraries(test_latency2 simgrid)
  target_link_libraries(test_latency3 simgrid)
  target_link_libraries(test_latency_bound simgrid)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_latency1.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_latency2.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_latency3.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_latency_bound.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_2p_1bb.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_2p_1fl.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_2p_1sl.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_2p_1switch.xml
  PARENT_SCOPE
  )
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_latency1.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test_latency2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test_latency3.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test_latency_bound.c
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
