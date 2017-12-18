# C examples
foreach(x actions-comm actions-storage
          app-pingpong app-token-ring
          async-wait async-waitall async-waitany
          cloud-sharing cloud-two-tasks cloud-simple
	  get_sender host_on_off host_on_off_recv host_on_off_processes	  
          process-daemon process-kill process-join process-lifetime process-migration process-suspend process-yield
          energy-consumption energy-ptask energy-pstate platform-properties
          io-file io-raw-storage io-file-remote
	  task-priority
	  plugin-hostload
          trace_integration)
  add_executable       (${x}  ${x}/${x}.c)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
endforeach()

# CPP examples
foreach(x task_destroy_cancel task_listen_from task_progress)
  add_executable       (${x}  ${x}/${x}.cpp)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.cpp)
endforeach()

add_executable       (bittorrent app-bittorrent/bittorrent.c app-bittorrent/messages.c app-bittorrent/peer.c app-bittorrent/tracker.c app-bittorrent/connection.c)
target_link_libraries(bittorrent simgrid)
set_target_properties(bittorrent PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/app-bittorrent)
foreach (file bittorrent connection messages peer tracker)
  set(teshsuite_src  ${teshsuite_src}  ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/${file}.c  ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/${file}.h)
endforeach()

add_executable       (chainsend app-chainsend/chainsend.c app-chainsend/iterator.c app-chainsend/common.c app-chainsend/messages.c app-chainsend/broadcaster.c app-chainsend/peer.c)
target_link_libraries(chainsend simgrid)
set_target_properties(chainsend PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/app-chainsend)
foreach (file common iterator messages broadcaster peer)
  set(teshsuite_src  ${teshsuite_src}  ${CMAKE_CURRENT_SOURCE_DIR}/app-chainsend/${file}.c  ${CMAKE_CURRENT_SOURCE_DIR}/app-chainsend/${file}.h)
endforeach()
set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/app-chainsend/chainsend.c)
                                  
set(teshsuite_src ${teshsuite_src}  PARENT_SCOPE)
set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/app-bittorrent.tesh          
                                   ${CMAKE_CURRENT_SOURCE_DIR}/app-chainsend/app-chainsend.tesh
                                   PARENT_SCOPE)
set(bin_files    ${bin_files}      ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/generate.py                  PARENT_SCOPE)
set(txt_files     ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/actions-comm.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/actions-comm_split_p0.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/actions-comm_split_p1.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/actions-storage/actions-storage.txt         PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/actions-comm_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/actions-comm_split_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/actions-storage/actions-storage_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/app-bittorrent_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/app-chainsend/app-chainsend_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/app-pingpong/app-pingpong_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait2_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait3_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait4_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/async-waitall/async-waitall_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/async-waitany/async-waitany_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/io-file-remote/io-file-remote_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/platform-properties/platform-properties_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/process-lifetime/baseline_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/process-lifetime/kill_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/process-lifetime/start_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/process-lifetime/start_kill_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/process-yield/process-yield_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/task-priority/task-priority_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/trace_integration/test-hbp1.0-hbp1.0-hbp1.0.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/trace_integration/test-hbp1.0-hbp3.0-hbp4.0.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/trace_integration/test-hbp1.5-hbp1.5.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/trace_integration/test-hbp1-c0s0-c0s1.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/trace_integration/test-hbp1-c0s0-c1s0.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/trace_integration/test-hbp1-c0s1-c0s2.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/trace_integration/test-hbp1-c0s1-c2s2.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/trace_integration/test-hbp1-c0s2-c1s0.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/trace_integration/test-hbp1-c0s2-c1s1.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/trace_integration/test-hbp1-c1s1-c1s2.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/trace_integration/test-hbp1-c1s1-c3s2.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/trace_integration/test-hbp2.5-hbp1.5.xml    PARENT_SCOPE)

foreach(x 
    actions-comm actions-storage
    async-wait async-waitall async-waitany
    app-bittorrent app-chainsend app-pingpong app-token-ring
    cloud-two-tasks cloud-simple
    energy-pstate
    host_on_off host_on_off_processes host_on_off_recv
    get_sender
    task_destroy_cancel task_listen_from task_progress 
    process-daemon process-kill process-join process-lifetime process-migration process-suspend process-yield
    energy-consumption energy-ptask
    io-file io-raw-storage io-file-remote
    platform-properties
    plugin-hostload
    trace_integration)
  ADD_TESH_FACTORIES(tesh-msg-${x} "thread;ucontext;raw;boost" 
                                   --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
				   --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/msg/${x}
				   --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/${x} 
				   ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/${x}/${x}.tesh)
endforeach()

foreach(x cloud-sharing)
  ADD_TESH(tesh-msg-${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/msg/${x} --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_BINARY_DIR}/teshsuite/msg/${x} ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/${x}/${x}.tesh)
endforeach()

ADD_TESH_FACTORIES(tesh-app-bittorrent-parallel         "thread;ucontext;raw;boost" --cfg contexts/nthreads:4 ${CONTEXTS_SYNCHRO} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/msg/app-bittorrent --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/app-bittorrent app-bittorrent.tesh)
