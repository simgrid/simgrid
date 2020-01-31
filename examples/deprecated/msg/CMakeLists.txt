# C examples
foreach(x app-masterworker cloud-masterworker
          dht-pastry energy-vm platform-failures 
          process-create 
          synchro-semaphore trace-categories 
          trace-route-user-variables trace-link-user-variables trace-masterworker
          trace-process-migration trace-host-user-variables)
  if(enable_msg)
    add_executable       (${x}  EXCLUDE_FROM_ALL ${x}/${x}.c)
    target_link_libraries(${x}  simgrid)
    set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
    add_dependencies(tests ${x})
  endif()
  set(examples_src ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
  set(tesh_files   ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
endforeach()

if(SIMGRID_HAVE_NS3 AND enable_msg)
  add_executable       (network-ns3 EXCLUDE_FROM_ALL network-ns3/network-ns3.c)
  target_link_libraries(network-ns3 simgrid)
  set_target_properties(network-ns3  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/network-ns3)
  add_dependencies(tests network-ns3)    
endif()

if(enable_msg)
  add_executable       (dht-kademlia EXCLUDE_FROM_ALL dht-kademlia/dht-kademlia.c dht-kademlia/node.c dht-kademlia/routing_table.c dht-kademlia/task.c dht-kademlia/answer.c)
  target_link_libraries(dht-kademlia simgrid)
  set_target_properties(dht-kademlia PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dht-kademlia)
  add_dependencies(tests dht-kademlia)
endif()
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
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/one_cluster_d.xml        PARENT_SCOPE)

if(enable_msg)
  foreach(x app-masterworker cloud-masterworker
            dht-pastry dht-kademlia platform-failures
            energy-vm
            process-create 
            synchro-semaphore)
    ADD_TESH_FACTORIES(msg-${x} "thread;ucontext;raw;boost" 
                                --setenv bindir=${CMAKE_BINARY_DIR}/examples/deprecated/msg/${x}
  			        --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/${x} 
			        --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms 
			        ${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/${x}/${x}.tesh)
  endforeach()

  foreach (x trace-categories trace-route-user-variables trace-link-user-variables trace-masterworker trace-process-migration trace-host-user-variables)
    ADD_TESH(msg-${x} --setenv bindir=${CMAKE_BINARY_DIR}/examples/deprecated/msg/${x}
	              --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/${x} 
	  	      --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms 
                      --cd ${CMAKE_BINARY_DIR}/examples/deprecated/msg/${x}
		      ${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/${x}/${x}.tesh)
  endforeach()

  if(CMAKE_SYSTEM_NAME STREQUAL "SunOS" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "i386" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    # Thread-local storage (TLS) is needed for parallel execution, but it doesn't
    # play well with Ucontexts on 64bit SunOS (at least on x86_64).
    set(parallel-factories "thread;raw;boost")
  else()
    set(parallel-factories "thread;ucontext;raw;boost")
  endif()
  ADD_TESH_FACTORIES(msg-dht-kademlia-parallel "${parallel-factories}" --cfg contexts/nthreads:4 ${CONTEXTS_SYNCHRO}
                                               --setenv bindir=${CMAKE_BINARY_DIR}/examples/deprecated/msg/dht-kademlia
                                               --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/dht-kademlia
                                               --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
		 	 		       ${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/dht-kademlia/dht-kademlia.tesh)

  ADD_TESH_FACTORIES(app-masterworker-multicore   "thread;ucontext;raw;boost" 
                                                  --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/app-masterworker 
                                                  --setenv bindir=${CMAKE_BINARY_DIR}/examples/deprecated/msg/app-masterworker
						  --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
						  ${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/app-masterworker/app-masterworker-multicore.tesh)
  ADD_TESH_FACTORIES(app-masterworker-vivaldi     "thread;ucontext;raw;boost" 
                                                  --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/app-masterworker 
                                                  --setenv bindir=${CMAKE_BINARY_DIR}/examples/deprecated/msg/app-masterworker
						  --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
						  ${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/app-masterworker/app-masterworker-vivaldi.tesh)

  if(SIMGRID_HAVE_NS3)
    ADD_TESH_FACTORIES(msg-network-ns3 "thread;ucontext;raw;boost"
                                        --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg
                                        --setenv bindir=${CMAKE_BINARY_DIR}/examples/deprecated/msg
                                        --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                                        ${CMAKE_HOME_DIRECTORY}/examples/deprecated/msg/network-ns3/network-ns3.tesh)
  endif()
endif(enable_msg)