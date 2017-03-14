# C examples
foreach(x actions-comm actions-storage app-masterworker app-pingpong app-pmm app-token-ring async-yield async-wait async-waitall 
          async-waitany cloud-capping cloud-masterworker cloud-migration cloud-simple cloud-two-tasks
          dht-chord dht-pastry energy-consumption energy-onoff energy-pstate energy-ptask energy-vm platform-failures 
          io-file io-remote io-storage task-priority 
          plugin-hostload process-create process-daemon process-join process-kill process-migration process-suspend 
          platform-properties process-startkilltime synchro-semaphore trace-categories 
          trace-route-user-variables trace-link-user-variables trace-masterworker trace-platform 
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

if(HAVE_NS3)
  add_executable       (network-ns3 network-ns3/network-ns3.c)
  target_link_libraries(network-ns3 simgrid)
  set_target_properties(network-ns3  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/network-ns3)
endif()

add_executable       (bittorrent app-bittorrent/bittorrent.c app-bittorrent/messages.c app-bittorrent/peer.c app-bittorrent/tracker.c app-bittorrent/connection.c)
target_link_libraries(bittorrent simgrid)
set_target_properties(bittorrent PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/app-bittorrent)
foreach (file bittorrent connection messages peer tracker)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/${file}.c  ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/${file}.h)
endforeach()

add_executable       (chainsend app-chainsend/chainsend.c app-chainsend/iterator.c app-chainsend/common.c app-chainsend/messages.c app-chainsend/broadcaster.c app-chainsend/peer.c)
target_link_libraries(chainsend simgrid)
set_target_properties(chainsend PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/app-chainsend)
foreach (file common iterator messages broadcaster peer)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/app-chainsend/${file}.c  ${CMAKE_CURRENT_SOURCE_DIR}/app-chainsend/${file}.h)
endforeach()

add_executable       (dht-kademlia dht-kademlia/dht-kademlia.c dht-kademlia/node.c dht-kademlia/routing_table.c dht-kademlia/task.c dht-kademlia/answer.c)
target_link_libraries(dht-kademlia simgrid)
set_target_properties(dht-kademlia PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dht-kademlia)
foreach (file answer dht-kademlia node routing_table task)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/${file}.c  ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/${file}.h)
endforeach()

foreach (file actions-comm actions-storage app-bittorrent app-chainsend app-masterworker app-pingpong async-yield async-wait
         async-waitall async-waitany dht-chord dht-kademlia io-remote platform-properties
         task-priority)
  set(xml_files    ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/${file}/${file}_d.xml)
endforeach()

set(txt_files    ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/README.doc
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/actions-comm.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/actions-comm_split_p0.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/actions-comm_split_p1.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-storage/actions-storage.txt          PARENT_SCOPE)
set(bin_files    ${bin_files}     ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/generate.py
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/generate.py
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/generate.py                     PARENT_SCOPE)
set(examples_src ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/app-chainsend/chainsend.c
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/dht-chord.h
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/common.h
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/network-ns3.c                    PARENT_SCOPE)
set(tesh_files   ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/app-bittorrent.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-chainsend/app-chainsend.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/dht-kademlia.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworker/app-masterworker-multicore.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworker/app-masterworker-vivaldi.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/network-ns3.tesh                 PARENT_SCOPE)
set(xml_files    ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/actions-comm_split_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworker/app-masterworker-multicore_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworker/app-masterworker-vivaldi_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait2_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait3_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait4_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/energy-onoff/platform_onoff.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/3hosts_2links_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/3links-timer_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/3links_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/crosstraffic_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/dogbone_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/onelink_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/one_cluster_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/network-ns3/two_clusters_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/process-startkilltime/baseline_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/process-startkilltime/kill_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/process-startkilltime/start_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/process-startkilltime/start_kill_d.xml       PARENT_SCOPE)

foreach(x actions-comm actions-storage app-bittorrent app-chainsend app-masterworker app-pingpong app-token-ring
          async-yield async-wait async-waitall async-waitany cloud-capping cloud-masterworker cloud-migration cloud-simple 
          cloud-two-tasks dht-chord dht-pastry dht-kademlia platform-failures io-file io-remote io-storage task-priority 
          process-create process-daemon process-join process-kill process-migration process-startkilltime process-suspend
	  platform-properties synchro-semaphore)
  ADD_TESH_FACTORIES(msg-${x} "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/${x} ${x}.tesh)
endforeach()

foreach (example consumption onoff pstate vm)
  ADD_TESH_FACTORIES(msg-energy-${example} "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/energy-${example}/energy-${example}.tesh)
endforeach()

foreach (x categories route-user-variables link-user-variables masterworker platform process-migration host-user-variables)
  ADD_TESH(msg-trace-${x} --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/trace-${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/trace-${x} trace-${x}.tesh)
endforeach()

ADD_TESH_FACTORIES(msg-app-bittorrent-parallel         "thread;ucontext;raw" --cfg contexts/nthreads:4 ${CONTEXTS_SYNCHRO} --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/app-bittorrent --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/app-bittorrent app-bittorrent.tesh)
ADD_TESH_FACTORIES(msg-dht-chord-parallel              "thread;ucontext;raw" --cfg contexts/nthreads:4 ${CONTEXTS_SYNCHRO} --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/dht-chord --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/dht-chord dht-chord.tesh)
ADD_TESH_FACTORIES(msg-dht-kademlia-parallel           "thread;ucontext;raw" --cfg contexts/nthreads:4 ${CONTEXTS_SYNCHRO} --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/dht-kademlia --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/dht-kademlia dht-kademlia.tesh)
ADD_TESH_FACTORIES(msg-energy-pstate-ptask             "thread;ucontext;raw;boost" --cfg host/model:ptask_L07 --log xbt_cfg.threshold:critical --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/energy-pstate/energy-pstate.tesh)
ADD_TESH_FACTORIES(msg-energy-consumption-ptask        "thread;ucontext;raw;boost" --cfg host/model:ptask_L07 --log xbt_cfg.threshold:critical --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/energy-consumption/energy-consumption.tesh)
ADD_TESH_FACTORIES(msg-energy-ptask                    "thread;ucontext;raw" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/energy-ptask/energy-ptask.tesh)
ADD_TESH_FACTORIES(msg-plugin-hostload "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/ ${CMAKE_HOME_DIRECTORY}/examples/msg/plugin-hostload/plugin-hostload.tesh)
if(NOT WIN32)
  ADD_TESH_FACTORIES(msg-maestro-set                     "thread" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/maestro-set/maestro-set.tesh)
else()
  message("Test maestro-set disabled on windows: this feature is not working")
endif()

ADD_TESH_FACTORIES(app-masterworker-multicore   "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/app-masterworker/app-masterworker-multicore.tesh)
ADD_TESH_FACTORIES(app-masterworker-vivaldi     "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/app-masterworker/app-masterworker-vivaldi.tesh)

ADD_TESH(msg-app-pmm  --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/app-pmm/app-pmm.tesh)

if(HAVE_NS3)
  ADD_TESH_FACTORIES(msg-network-ns3 "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/network-ns3/network-ns3.tesh)
endif()
