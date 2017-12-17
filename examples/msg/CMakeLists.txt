# C examples
foreach(x app-masterworker 
          cloud-capping cloud-masterworker cloud-migration
          dht-pastry energy-vm platform-failures 
          process-create 
          synchro-semaphore trace-categories 
          trace-route-user-variables trace-link-user-variables trace-masterworker
          trace-process-migration trace-host-user-variables)
  add_executable       (${x}     ${x}/${x}.c)
  target_link_libraries(${x}     simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
  set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
endforeach()

# CPP examples
foreach(x maestro-set)
  add_executable       (${x}     ${x}/${x}.cpp)
  target_link_libraries(${x}     simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
  set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.cpp)
  set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
endforeach()

if(SIMGRID_HAVE_NS3)
  add_executable       (network-ns3 network-ns3/network-ns3.c)
  target_link_libraries(network-ns3 simgrid)
  set_target_properties(network-ns3  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/network-ns3)
endif()

add_executable       (dht-kademlia dht-kademlia/dht-kademlia.c dht-kademlia/node.c dht-kademlia/routing_table.c dht-kademlia/task.c dht-kademlia/answer.c)
target_link_libraries(dht-kademlia simgrid)
set_target_properties(dht-kademlia PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dht-kademlia)
foreach (file answer dht-kademlia node routing_table task)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/${file}.c  ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/${file}.h)
endforeach()

foreach (file app-masterworker dht-kademlia dht-pastry)
  set(xml_files    ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/${file}/${file}_d.xml)
endforeach()

set(txt_files    ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/README.doc          PARENT_SCOPE)
set(bin_files    ${bin_files}     ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/generate.py
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-pastry/generate.py                       PARENT_SCOPE)
set(examples_src ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/common.h
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/network-ns3.c                    PARENT_SCOPE)
set(tesh_files   ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/dht-kademlia.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworker/app-masterworker-multicore.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworker/app-masterworker-vivaldi.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/network-ns3.tesh                 PARENT_SCOPE)
set(xml_files    ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworker/app-masterworker-multicore_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworker/app-masterworker-vivaldi_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/3hosts_2links_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/3links-timer_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/3links_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/crosstraffic_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/dogbone_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/onelink_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/one_cluster_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/two_clusters_d.xml       PARENT_SCOPE)

foreach(x app-masterworker 
          cloud-capping cloud-masterworker cloud-migration
          dht-pastry dht-kademlia platform-failures
          energy-vm
          process-create 
          synchro-semaphore)
  ADD_TESH_FACTORIES(msg-${x} "thread;ucontext;raw;boost" 
                              --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/${x}
			      --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg/${x} 
			      --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms 
			      ${CMAKE_HOME_DIRECTORY}/examples/msg/${x}/${x}.tesh)
endforeach()

foreach (x trace-categories trace-route-user-variables trace-link-user-variables trace-masterworker trace-process-migration trace-host-user-variables)
  ADD_TESH(msg-${x} --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/${x}
	            --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg/${x} 
		    --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms 
                    --cd ${CMAKE_BINARY_DIR}/examples/msg/${x}
		    ${CMAKE_HOME_DIRECTORY}/examples/msg/${x}/${x}.tesh)
endforeach()

ADD_TESH_FACTORIES(msg-dht-kademlia-parallel "thread;ucontext;raw;boost" --cfg contexts/nthreads:4 ${CONTEXTS_SYNCHRO} 
                                             --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/dht-kademlia 
					     --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg/dht-kademlia
					     --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
					     ${CMAKE_HOME_DIRECTORY}/examples/msg/dht-kademlia/dht-kademlia.tesh)

if(NOT WIN32)
  ADD_TESH_FACTORIES(msg-maestro-set   "thread" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/maestro-set/maestro-set.tesh)
else()
  message("Test maestro-set disabled on windows: this feature is not working")
endif()

ADD_TESH_FACTORIES(app-masterworker-multicore   "thread;ucontext;raw;boost" 
                                                --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg/app-masterworker 
                                                --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/app-masterworker
						--setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
						${CMAKE_HOME_DIRECTORY}/examples/msg/app-masterworker/app-masterworker-multicore.tesh)
ADD_TESH_FACTORIES(app-masterworker-vivaldi     "thread;ucontext;raw;boost" 
                                                --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg/app-masterworker 
                                                --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/app-masterworker
						--setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
						${CMAKE_HOME_DIRECTORY}/examples/msg/app-masterworker/app-masterworker-vivaldi.tesh)

if(SIMGRID_HAVE_NS3)
  ADD_TESH_FACTORIES(msg-network-ns3 "thread;ucontext;raw;boost"
                                      --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg
                                      --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg
                                      --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                                      ${CMAKE_HOME_DIRECTORY}/examples/msg/network-ns3/network-ns3.tesh)
endif()
