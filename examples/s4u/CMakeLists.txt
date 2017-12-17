foreach (example actor-create actor-daemon actor-join actor-kill actor-lifetime actor-migration actor-suspend actor-yield
                 app-chainsend app-masterworker app-pingpong app-token-ring
                 async-wait async-waitany async-waitall
                 cloud-simple
                 energy-exec energy-boot energy-link energy-vm
                 exec-async exec-basic exec-dvfs exec-monitor exec-ptask exec-remote
                 io-file-system io-file-remote io-storage-raw
                 mutex
                 platform-properties plugin-hostload 
                 replay-comm replay-storage
                 trace-platform)
  add_executable       (s4u-${example}  ${example}/s4u-${example}.cpp)
  target_link_libraries(s4u-${example}  simgrid)
  set_target_properties(s4u-${example}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${example})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${example}/s4u-${example}.tesh)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/${example}/s4u-${example}.cpp)
endforeach()

# CHORD EXAMPLE
add_executable       (s4u-dht-chord dht-chord/s4u-dht-chord.cpp dht-chord/node.cpp)
target_link_libraries(s4u-dht-chord simgrid)
set_target_properties(s4u-dht-chord PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dht-chord)
foreach (file s4u-dht-chord node)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/${file}.cpp)
endforeach()
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/s4u-dht-chord.hpp)

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
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/s4u-dht-chord.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actor-lifetime/s4u-actor-lifetime.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actor-yield/s4u-actor-yield.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/s4u-async-wait.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-waitany/s4u-async-waitany.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-waitall/s4u-async-waitall.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/exec-basic/s4u-exec-basic.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/exec-dvfs/s4u-exec-dvfs.tesh
                  PARENT_SCOPE)
set(xml_files     ${xml_files}    ${CMAKE_CURRENT_SOURCE_DIR}/actor-create/s4u-actor-create_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actor-lifetime/s4u-actor-lifetime_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actor-yield/s4u-actor-yield_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/s4u-app-bittorrent_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworker/s4u-app-masterworker_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-waitany/s4u-async-waitany_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-waitall/s4u-async-waitall_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/s4u-async-wait_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/s4u-dht-chord_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/energy-boot/platform_boot.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/io-file-remote/s4u-io-file-remote_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/platform-properties/s4u-platform-properties_d.xml
				  ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm-split_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-storage/s4u-replay-storage_d.xml
		  PARENT_SCOPE)
set(txt_files     ${txt_files}    ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm-split-p0.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm-split-p1.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-comm/s4u-replay-comm.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/replay-storage/s4u-replay-storage.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/README.doc                                   PARENT_SCOPE)

foreach(example actor-create actor-daemon actor-join actor-kill actor-lifetime actor-migration actor-suspend actor-yield
                app-bittorrent app-chainsend app-masterworker app-pingpong app-token-ring 
                async-wait async-waitall async-waitany
                cloud-simple
                dht-chord 
                energy-exec energy-boot energy-link energy-vm
                exec-async exec-basic exec-dvfs exec-monitor exec-ptask exec-remote
                platform-properties plugin-hostload mutex
                io-file-system io-file-remote io-storage-raw
                replay-comm replay-storage)
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
