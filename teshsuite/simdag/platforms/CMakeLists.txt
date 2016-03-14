foreach(x basic_parsing_test basic_link_test evaluate_parse_time evaluate_get_route_time)
  add_executable       (${x} ${x}.c)
  target_link_libraries(${x} simgrid)

  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}.c)
endforeach()

add_executable       (is_router_test          is_router_test.cpp)
target_link_libraries(is_router_test          simgrid)
add_executable       (flatifier               flatifier.cpp)
target_link_libraries(flatifier               simgrid)

set(tesh_files    ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/basic_parsing_test_failing.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/basic_parsing_test_sym_full.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/basic_parsing_test.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/basic_parsing_test_bypass.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/basic_link_test.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/flatifier.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/get_full_link.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/bogus_two_hosts_asymetric.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/bogus_missing_gateway.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/bogus_disk_attachment.tesh                 PARENT_SCOPE)
set(xml_files     ${xml_files}      ${CMAKE_CURRENT_SOURCE_DIR}/four_hosts_Dijkstra_ns3.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/four_hosts_floyd_ns3.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/four_hosts_floyd.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/two_hosts_multi_hop.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/host_attributes.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/link_attributes.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/one_cluster_file.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/one_cluster_multicore.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/one_cluster_fullduplex.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/one_cluster_router_id.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/one_cluster.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platform_include.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/properties.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/test_of_is_router.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/three_hosts_non_symmetric_route.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/two_clusters_one_name.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/two_clusters_router_id.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/two_clusters.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/bogus_missing_src_gateway.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/bogus_missing_dst_gateway.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/bogus_two_hosts_asymetric.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/two_hosts_one_link_fullduplex.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/two_hosts_one_link_symmetrical.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/two_hosts_one_link.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/clusterA.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/clusterB.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/Dijkstra.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/bogus_disk_attachment.xml                  PARENT_SCOPE)
set(teshsuite_src ${teshsuite_src}  ${CMAKE_CURRENT_SOURCE_DIR}/flatifier.cpp
                                    ${CMAKE_CURRENT_SOURCE_DIR}/is_router_test.cpp                         PARENT_SCOPE)
set(txt_files     ${txt_files}      ${CMAKE_CURRENT_SOURCE_DIR}/carol.fail
                                    ${CMAKE_CURRENT_SOURCE_DIR}/bob.trace
                                    ${CMAKE_CURRENT_SOURCE_DIR}/erin.avail
                                    ${CMAKE_CURRENT_SOURCE_DIR}/link.bw
                                    ${CMAKE_CURRENT_SOURCE_DIR}/link.fail
                                    ${CMAKE_CURRENT_SOURCE_DIR}/link.lat                                   PARENT_SCOPE)
