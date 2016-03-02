add_executable       (test_latency1      test_latency1.c)
target_link_libraries(test_latency1      simgrid)
add_executable       (test_latency2      test_latency2.c)
target_link_libraries(test_latency2      simgrid)
add_executable       (test_latency3      test_latency3.c)
target_link_libraries(test_latency3      simgrid)
add_executable       (test_latency_bound test_latency_bound.c)
target_link_libraries(test_latency_bound simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_latency1.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_latency2.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_latency3.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_latency_bound.tesh
  PARENT_SCOPE)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_2p_1bb.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_2p_1fl.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_2p_1sl.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_2p_1switch.xml
  PARENT_SCOPE)
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_latency1.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test_latency2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test_latency3.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test_latency_bound.c
  PARENT_SCOPE)
