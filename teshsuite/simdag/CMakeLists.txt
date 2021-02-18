foreach(x availability basic0 basic1 basic3 basic4 basic5 comm-p2p-latency-bound incomplete)
  add_executable       (${x}  EXCLUDE_FROM_ALL ${x}/${x}.c)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  add_dependencies(tests ${x})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
endforeach()

foreach(x flatifier)
  add_executable       (${x} EXCLUDE_FROM_ALL ${x}/${x}.cpp)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  set_property(TARGET ${x} APPEND PROPERTY INCLUDE_DIRECTORIES "${INTERNAL_INCLUDES}")
  add_dependencies(tests ${x})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.cpp)
endforeach()

set(teshsuite_src ${teshsuite_src}  PARENT_SCOPE)
set(tesh_files    ${tesh_files}     ${CMAKE_CURRENT_SOURCE_DIR}/flatifier/bogus_two_hosts_asymetric.tesh
                                    ${CMAKE_CURRENT_SOURCE_DIR}/flatifier/bogus_missing_gateway.tesh
                                    PARENT_SCOPE)
set(xml_files     ${xml_files}      ${CMAKE_CURRENT_SOURCE_DIR}/platforms/four_hosts_floyd.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/two_hosts_multi_hop.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/host_attributes.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/link_attributes.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/one_cluster_multicore.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/one_cluster_splitduplex.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/one_cluster_router_id.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/one_cluster.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/platform_4p_1switch.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/properties.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/test_of_is_router.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/three_hosts_non_symmetric_route.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/two_clusters_one_name.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/two_clusters_router_id.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/two_clusters.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/bogus_missing_src_gateway.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/bogus_missing_dst_gateway.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/bogus_two_hosts_asymetric.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/two_hosts_one_link_splitduplex.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/two_hosts_one_link.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/Dijkstra.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/platform_2p_1bb.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/platform_2p_1fl.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/platform_2p_1sl.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/platform_2p_1sl_hetero.xml
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/platform_2p_1switch.xml          PARENT_SCOPE)
set(txt_files     ${txt_files}      ${CMAKE_CURRENT_SOURCE_DIR}/platforms/carol.fail
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/bob.trace
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/erin.avail
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/link.bw
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/link.fail
                                    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/link.lat                         PARENT_SCOPE)

foreach(x availability basic0 basic1 basic3 basic4 basic5 flatifier comm-p2p-latency-bound incomplete)
  ADD_TESH(tesh-simdag-${x} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simdag/${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/${x} ${x}.tesh)
endforeach()

if(enable_debug)
  # these tests need assertions
  ADD_TESH(tesh-parser-bogus-symmetric         --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simdag/flatifier --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/flatifier ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/flatifier/bogus_two_hosts_asymetric.tesh)
  ADD_TESH(tesh-parser-bogus-missing-gw        --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simdag/flatifier --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/flatifier ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/flatifier/bogus_missing_gateway.tesh)

   #These tests fail on Windows as the signal returned is not the same as Unix would send.
  if(WIN32)
    set_property(TEST tesh-parser-bogus-symmetric PROPERTY WILL_FAIL TRUE)
    set_property(TEST tesh-parser-bogus-missing-gw PROPERTY WILL_FAIL TRUE)
  endif()
endif()
