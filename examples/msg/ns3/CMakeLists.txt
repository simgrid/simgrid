if(HAVE_NS3)
  add_executable       (ns3 ns3.c)
  target_link_libraries(ns3 simgrid)

  ADD_TESH_FACTORIES(msg-ns3 "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/ns3/ns3.tesh)
endif()

set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/ns3.c                   PARENT_SCOPE)
set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/ns3.tesh                PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/3hosts_2links_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/3links-d-timer.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/3links-d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/dogbone-d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/One_cluster-d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/Two_clusters-d.xml      PARENT_SCOPE)
