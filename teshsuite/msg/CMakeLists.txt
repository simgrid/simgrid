# C examples
foreach(x actions-comm actions-storage cloud-sharing get_sender host_on_off host_on_off_recv host_on_off_processes 
          trace_integration)
  add_executable       (${x}  ${x}/${x}.c)
  target_link_libraries(${x}  simgrid)
  set_target_properties(${x}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
endforeach()

# CPP examples
foreach(x task_destroy_cancel task_listen_from)
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


set(teshsuite_src ${teshsuite_src}  PARENT_SCOPE)
set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/app-bittorrent.tesh          PARENT_SCOPE)
set(bin_files    ${bin_files}      ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/generate.py                  PARENT_SCOPE)
set(txt_files     ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/actions-comm.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/actions-comm_split_p0.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/actions-comm_split_p1.txt
                                   ${CMAKE_CURRENT_SOURCE_DIR}/actions-storage/actions-storage.txt         PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/actions-comm_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/actions-comm/actions-comm_split_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/actions-storage/actions-storage_d.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/app-bittorrent_d.xml
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

foreach(x get_sender host_on_off host_on_off_processes host_on_off_recv task_destroy_cancel task_listen_from trace_integration)
  ADD_TESH_FACTORIES(tesh-msg-${x} "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/msg/${x} --cd ${CMAKE_BINARY_DIR}/teshsuite/msg/${x} ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/${x}/${x}.tesh)
endforeach()

foreach(x actions-comm actions-storage app-bittorrent)
  ADD_TESH_FACTORIES(tesh-msg-${x} "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/msg/${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/${x} ${x}.tesh)
endforeach()
# One context factory is enough for these ones

foreach(x cloud-sharing)
  ADD_TESH(tesh-msg-${x} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/msg/${x} --cd ${CMAKE_BINARY_DIR}/teshsuite/msg/${x} ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/${x}/${x}.tesh)
endforeach()

ADD_TESH_FACTORIES(tesh-app-bittorrent-parallel         "thread;ucontext;raw;boost" --cfg contexts/nthreads:4 ${CONTEXTS_SYNCHRO} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/msg/app-bittorrent --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/app-bittorrent app-bittorrent.tesh)
