foreach (example actions-comm actions-storage 
                 actor-create actor-daemon actor-kill actor-migration actor-suspend 
                 app-masterworker app-pingpong app-token-ring
		 async-wait async-waitany async-waitall
		 plugin-hostload io mutex)
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
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/s4u-async-wait.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-waitany/s4u-async-waitany.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-waitall/s4u-async-waitall.tesh         PARENT_SCOPE)
set(xml_files     ${xml_files}    ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/s4u-actions-comm-split_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/s4u-actions-comm_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-storage/s4u-actions-storage_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actor-create/s4u-actor-create_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/s4u-app-bittorrent_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworker/s4u-app-masterworker_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-waitany/s4u-async-waitany_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-waitall/s4u-async-waitall_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/s4u-async-wait_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/s4u-dht-chord_d.xml                PARENT_SCOPE)
set(txt_files     ${txt_files}    ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/s4u-actions-comm-split-p0.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/s4u-actions-comm-split-p1.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/s4u-actions-comm.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-storage/s4u-actions-storage.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/README.doc                                   PARENT_SCOPE)

foreach(example actions-comm actions-storage 
                actor-create actor-daemon actor-kill actor-migration actor-suspend
                app-bittorrent app-masterworker app-pingpong app-token-ring 
		async-wait async-waitall async-waitany
		dht-chord plugin-hostload io mutex)
  ADD_TESH_FACTORIES(s4u-${example} "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}/${example} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/examples/s4u/${example} s4u-${example}.tesh)
endforeach()
