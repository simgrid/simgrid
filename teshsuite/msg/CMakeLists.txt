# C examples
foreach(x async-wait
          cloud-migration cloud-two-tasks
          get_sender host_on_off host_on_off_recv
          process-lifetime
          energy-ptask energy-pstate platform-properties
          io-file io-file-remote
          task-priority
          trace_integration)
  if(enable_msg)
    add_executable       (${x} EXCLUDE_FROM_ALL ${x}/${x}.c)
    target_link_libraries(${x} simgrid)
    set_target_properties(${x} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
    add_dependencies(tests ${x})
  endif()

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
endforeach()

# CPP examples
foreach(x task_destroy_cancel task_listen_from task_progress host_on_off_processes)
  if(enable_msg)
    add_executable       (${x}  EXCLUDE_FROM_ALL ${x}/${x}.cpp)
    target_link_libraries(${x}  simgrid)
    set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
    add_dependencies(tests ${x})
  endif()

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.cpp)
endforeach()

if(enable_msg)
  add_executable       (bittorrent EXCLUDE_FROM_ALL app-bittorrent/bittorrent.c app-bittorrent/bittorrent-messages.c app-bittorrent/bittorrent-peer.c app-bittorrent/tracker.c app-bittorrent/connection.c)
  target_link_libraries(bittorrent simgrid)
  set_target_properties(bittorrent PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/app-bittorrent)
  add_dependencies(tests bittorrent)
endif()
foreach (file bittorrent connection bittorrent-messages bittorrent-peer tracker)
  set(teshsuite_src  ${teshsuite_src}  ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/${file}.c  ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/${file}.h)
endforeach()

set(teshsuite_src ${teshsuite_src}  PARENT_SCOPE)
set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/app-bittorrent.tesh          
                                   PARENT_SCOPE)
set(bin_files    ${bin_files}      ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/generate.py                  PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/app-bittorrent_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait2_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait3_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/async-wait/async-wait4_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/io-file-remote/io-file-remote_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/platform-properties/platform-properties_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/process-lifetime/baseline_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/process-lifetime/kill_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/process-lifetime/start_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/process-lifetime/start_kill_d.xml
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

if(enable_msg)
  foreach(x 
    async-wait
    app-bittorrent
    cloud-migration cloud-two-tasks
    energy-pstate
    host_on_off host_on_off_processes host_on_off_recv
    get_sender
    task_destroy_cancel task_listen_from task_progress 
    process-lifetime
    energy-ptask
    io-file io-file-remote
    platform-properties
    task-priority
    trace_integration)
    
    ADD_TESH_FACTORIES(tesh-msg-${x} "raw" 
                                     --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
 				     --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/msg/${x}
				     --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/${x} 
				     ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/${x}/${x}.tesh)
  endforeach()


  ADD_TESH_FACTORIES(tesh-app-bittorrent-parallel         "raw" --cfg contexts/nthreads:4 ${CONTEXTS_SYNCHRO} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/msg/app-bittorrent --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/app-bittorrent app-bittorrent.tesh)
endif()
