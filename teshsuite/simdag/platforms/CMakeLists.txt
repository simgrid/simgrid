cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(basic_parsing_test basic_parsing_test.c)
add_executable(is_router_test is_router_test.c)
add_executable(flatifier flatifier.c)
add_executable(basic_tracing basic_tracing.c)
add_executable(basic_link_test basic_link_test.c)

### Add definitions for compile
if(NOT WIN32)
add_executable(evaluate_parse_time Evaluate_parse_time.c)
add_executable(evaluate_get_route_time Evaluate_get_route_time.c)
target_link_libraries(evaluate_get_route_time simgrid m)
target_link_libraries(evaluate_parse_time simgrid m)
target_link_libraries(basic_parsing_test simgrid m)
target_link_libraries(is_router_test simgrid m)
target_link_libraries(flatifier simgrid m)
target_link_libraries(basic_tracing simgrid m)
target_link_libraries(basic_link_test simgrid m)
else(NOT WIN32)
target_link_libraries(basic_parsing_test simgrid)
target_link_libraries(is_router_test simgrid)
target_link_libraries(flatifier simgrid)
target_link_libraries(basic_tracing simgrid)
target_link_libraries(basic_link_test simgrid)
endif(NOT WIN32)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/basic_parsing_test_failing.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/basic_parsing_test_sym_full.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/basic_parsing_test.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/basic_parsing_test_bypass.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/basic_link_test.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/flatifier.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/get_full_link.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/two_hosts_asymetric.tesh
  PARENT_SCOPE
)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/four_hosts_Dijkstra_ns3.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/four_hosts_floyd_bis.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/four_hosts_floyd_ns3.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/four_hosts_floyd.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/two_hosts_multi_hop.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_cluster_file.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_cluster_foreach.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_cluster_fullduplex.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_cluster_one_host_random.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_cluster_one_host.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_cluster_properties_foreach.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_cluster_properties_override.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_cluster_router_id.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_cluster.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_host_availability.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_host_properties.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_host_state_file.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_host_state_off.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_host_state_on.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_host_trace_file.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_host_trace_inside.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_host.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_link_availability.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_link_fatpipe.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_link_shared.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/one_link_state_file.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_include.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/properties.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/random.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/route_override.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/test_of_is_router.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/three_hosts_non_symmetric_route.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/two_clusters_one_name.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/two_clusters_router_id.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Two_clusters_rulebased.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/two_clusters_symmetric.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/two_clusters.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/two_hosts_asymetric_BOGUS2.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/two_hosts_asymetric_BOGUS.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/two_hosts_one_link_fullduplex.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/two_hosts_one_link_symmetrical.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/two_hosts_one_link.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/clusterA.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/clusterB.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Dijkstra.xml
  PARENT_SCOPE
)
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/basic_parsing_test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/basic_tracing.c
  ${CMAKE_CURRENT_SOURCE_DIR}/basic_link_test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Evaluate_get_route_time.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Evaluate_parse_time.c
  ${CMAKE_CURRENT_SOURCE_DIR}/flatifier.c
  ${CMAKE_CURRENT_SOURCE_DIR}/is_router_test.c
  PARENT_SCOPE
)
set(bin_files
  ${bin_files}
  PARENT_SCOPE
)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/bob0_availability_file.trace
  ${CMAKE_CURRENT_SOURCE_DIR}/bob0_state_file.trace
  ${CMAKE_CURRENT_SOURCE_DIR}/bob2_availability_file.trace
  ${CMAKE_CURRENT_SOURCE_DIR}/bob2_state_file.trace
  ${CMAKE_CURRENT_SOURCE_DIR}/bob.fail
  ${CMAKE_CURRENT_SOURCE_DIR}/bob.trace
  ${CMAKE_CURRENT_SOURCE_DIR}/link1.bw
  ${CMAKE_CURRENT_SOURCE_DIR}/link1.fail
  ${CMAKE_CURRENT_SOURCE_DIR}/link1.lat
  PARENT_SCOPE
)