# THIS IS ONLY FOR THE FILES. To add your test (tesh) as well, make sure you add
# the same at the bottom of the file as well.
foreach (example actor-create actor-daemon actor-join actor-kill
                 actor-lifetime actor-migration actor-suspend actor-yield
                 app-chainsend app-pingpong app-token-ring
                 async-ready async-wait async-waitany async-waitall async-waituntil
                 cloud-capping cloud-migration cloud-simple
                 energy-exec energy-boot energy-link energy-vm
                 engine-filtering
                 exec-async exec-basic exec-dvfs exec-monitor exec-ptask exec-remote
                 io-async io-file-system io-file-remote io-storage-raw
                 platform-failures platform-properties plugin-hostload 
                 replay-comm replay-storage
                 routing-get-clusters
                 synchro-barrier synchro-mutex synchro-semaphore
                 trace-platform)
  add_executable       (s4u-${example}  ${example}/s4u-${example}.cpp)
  target_link_libraries(s4u-${example}  simgrid)
  set_target_properties(s4u-${example}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${example})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${example}/s4u-${example}.tesh)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/${example}/s4u-${example}.cpp)
endforeach()

# MASTERWORKERS EXAMPLE
foreach(variant fun class) 
  add_executable       (s4u-app-masterworkers-${variant}  app-masterworkers/s4u-app-masterworkers-${variant}.cpp)
  target_link_libraries(s4u-app-masterworkers-${variant}  simgrid)
  set_target_properties(s4u-app-masterworkers-${variant}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/app-masterworkers)

  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworkers/s4u-app-masterworkers-${variant}.cpp)
endforeach()
set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworkers/s4u-app-masterworkers.tesh)

# CHORD EXAMPLE
add_executable       (s4u-dht-chord dht-chord/s4u-dht-chord.cpp dht-chord/s4u-dht-chord-node.cpp)
target_link_libraries(s4u-dht-chord simgrid)
set_target_properties(s4u-dht-chord PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dht-chord)
foreach (file s4u-dht-chord s4u-dht-chord-node)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/${file}.cpp)
endforeach()
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/s4u-dht-chord.hpp)

# KADEMLIA EXAMPLE
add_executable       (s4u-dht-kademlia dht-kademlia/s4u-dht-kademlia.cpp dht-kademlia/node.cpp 
                      dht-kademlia/routing_table.cpp dht-kademlia/answer.cpp)
target_link_libraries(s4u-dht-kademlia simgrid)
set_target_properties(s4u-dht-kademlia PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dht-kademlia)
foreach (file answer routing_table node s4u-dht-kademlia)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/${file}.cpp 
                                     ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/${file}.hpp)
endforeach()
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/message.hpp)

# BITTORRENT EXAMPLE
add_executable       (s4u-bittorrent app-bittorrent/s4u-bittorrent.cpp app-bittorrent/s4u-peer.cpp
                      app-bittorrent/s4u-tracker.cpp)
target_link_libraries(s4u-bittorrent simgrid)
set_target_properties(s4u-bittorrent PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/app-bittorrent)
foreach (file s4u-bittorrent s4u-peer s4u-tracker)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/${file}.cpp
                                     ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/${file}.hpp)
endforeach()

set(examples_src  ${examples_src}                                                                          PARENT_SCOPE)
set(tesh_files    ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/s4u-app-bittorrent.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-pingpong/simix-breakpoint.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/s4u-dht-chord.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/s4u-dht-kademlia.tesh
                  PARENT_SCOPE)
set(xml_files     ${xml_files}    ${CMAKE_CURRENT_SOURCE_DIR}/actor-create/s4u-actor-create_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actor-lifetime/s4u-actor-lifetime_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actor-yield/s4u-actor-yield_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/s4u-app-bittorrent_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworkers/s4u-app-masterworkers_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/s4u-async-wait_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-waitany/s4u-async-waitany_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-waitall/s4u-async-waitall_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/s4u-async-wait_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-ready/s4u-async-ready_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-waituntil/s4u-async-waituntil_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/s4u-dht-chord_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/s4u-dht-kademlia_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/energy-boot/platform_boot.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/io-file-remote/s4u-io-file-remote_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/platform-properties/s4u-platform-properties_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/platform-failures/s4u-masterworker-failures_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm-split_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-storage/s4u-replay-storage_d.xml
                  PARENT_SCOPE)
set(bin_files     ${bin_files}    ${CMAKE_CURRENT_SOURCE_DIR}/dht-kademlia/generate.py                     PARENT_SCOPE)
set(txt_files     ${txt_files}    ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm-split-p0.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm-split-p1.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-storage/s4u-replay-storage.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/README.rst                                   PARENT_SCOPE)

foreach(example actor-create actor-daemon actor-join actor-kill
                actor-lifetime actor-migration actor-suspend actor-yield
                app-bittorrent app-chainsend app-masterworkers app-pingpong app-token-ring 
                async-ready async-wait async-waitall async-waitany async-waituntil
                cloud-capping cloud-migration cloud-simple
                dht-chord dht-kademlia
                energy-exec energy-boot energy-link energy-vm
                engine-filtering
                exec-async exec-basic exec-dvfs exec-monitor exec-ptask exec-remote
                platform-properties plugin-hostload # FIXME: platform-failures is disabled
                io-async io-file-system io-file-remote io-storage-raw
                replay-comm replay-storage
                routing-get-clusters
                synchro-barrier synchro-mutex synchro-semaphore
                )
  ADD_TESH_FACTORIES(s4u-${example} "thread;ucontext;raw;boost" 
                                    --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}/${example} 
				    --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms 
				    --cd ${CMAKE_CURRENT_SOURCE_DIR}/${example} 
				    ${CMAKE_HOME_DIRECTORY}/examples/s4u/${example}/s4u-${example}.tesh)
endforeach()

foreach (example trace-platform)
  ADD_TESH(s4u-${example} --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}/${example}
                          --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms 
                          ${CMAKE_HOME_DIRECTORY}/examples/s4u/${example}/s4u-${example}.tesh)
endforeach()

if (NOT enable_memcheck AND NOT WIN32)
  ADD_TESH(simix-breakpoint --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}/app-pingpong
                            --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                            ${CMAKE_CURRENT_SOURCE_DIR}/app-pingpong/simix-breakpoint.tesh)
endif()
