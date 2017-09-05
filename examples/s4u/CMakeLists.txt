foreach (example actions-comm actions-storage actor-create actor-daemon actor-kill actor-migration actor-suspend 
                 app-masterworker app-pingpong app-token-ring plugin-hostload io mutex async-waitany async-waitall)
  add_executable       (s4u_${example}  ${example}/s4u_${example}.cpp)
  target_link_libraries(s4u_${example}  simgrid)
  set_target_properties(s4u_${example}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${example})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${example}/s4u_${example}.tesh)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/${example}/s4u_${example}.cpp)
endforeach()

# CHORD EXAMPLE
add_executable       (s4u_dht-chord dht-chord/s4u_dht-chord.cpp dht-chord/node.cpp)
target_link_libraries(s4u_dht-chord simgrid)
set_target_properties(s4u_dht-chord PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dht-chord)
foreach (file s4u_dht-chord node)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/${file}.cpp)
endforeach()
set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/s4u_dht-chord.hpp)

add_executable       (s4u_bittorrent app-bittorrent/s4u_bittorrent.cpp app-bittorrent/s4u_peer.cpp
                      app-bittorrent/s4u_tracker.cpp)
target_link_libraries(s4u_bittorrent simgrid)
set_target_properties(s4u_bittorrent PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/app-bittorrent)
foreach (file s4u_bittorrent s4u_peer s4u_tracker)
  set(examples_src  ${examples_src}  ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/${file}.cpp
                                     ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/${file}.hpp)
endforeach()

set(examples_src  ${examples_src}                                                                          PARENT_SCOPE)
set(tesh_files    ${tesh_files}   ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/s4u_app-bittorrent.tesh
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/s4u_dht-chord.tesh                 PARENT_SCOPE)
set(xml_files     ${xml_files}    ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/s4u_actions-comm_split_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/s4u_actions-comm_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-storage/s4u_actions-storage_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actor-create/s4u_actor-create_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/s4u_app-bittorrent_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/app-masterworker/s4u_app-masterworker_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-waitany/s4u_async-waitany_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/async-waitany/s4u_async-waitall_d.xml
                                  ${CMAKE_CURRENT_SOURCE_DIR}/dht-chord/s4u_dht-chord_d.xml                PARENT_SCOPE)
set(txt_files     ${txt_files}    ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/s4u_actions-comm_split_p0.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/s4u_actions-comm_split_p1.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/s4u_actions-comm.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/actions-storage/s4u_actions-storage.txt
                                  ${CMAKE_CURRENT_SOURCE_DIR}/README.doc                                   PARENT_SCOPE)

foreach(example actions-comm actions-storage actor-create actor-daemon actor-kill actor-migration actor-suspend 
                app-bittorrent app-masterworker app-pingpong app-token-ring dht-chord plugin-hostload io mutex )
  ADD_TESH_FACTORIES(s4u-${example} "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_CURRENT_BINARY_DIR}/${example} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/examples/s4u/${example} s4u_${example}.tesh)
endforeach()
