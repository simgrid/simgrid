# C examples
foreach(x cloud-two-tasks get_sender io-file task_listen_from task_destroy_cancel)
  if(enable_msg)
    add_executable       (${x} EXCLUDE_FROM_ALL ${x}/${x}.c)
    target_link_libraries(${x} simgrid)
    set_target_properties(${x} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${x})
    add_dependencies(tests ${x})
  endif()

  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.tesh)
  set(teshsuite_src ${teshsuite_src} ${CMAKE_CURRENT_SOURCE_DIR}/${x}/${x}.c)
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
set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/app-bittorrent.tesh          PARENT_SCOPE)
set(bin_files     ${bin_files}     ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/generate.py                  PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/app-bittorrent/app-bittorrent_d.xml
                                   PARENT_SCOPE)

if(enable_msg)
  foreach(x app-bittorrent cloud-two-tasks get_sender task_destroy_cancel task_listen_from io-file)

    ADD_TESH_FACTORIES(tesh-msg-${x} "raw"  --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms
                                            --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/msg/${x}
                                            --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/${x} 
                                            ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/${x}/${x}.tesh)
  endforeach()

  ADD_TESH_FACTORIES(tesh-app-bittorrent-parallel         "raw" --cfg contexts/nthreads:4 ${CONTEXTS_SYNCHRO} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/msg/app-bittorrent --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --setenv platfdir=${CMAKE_HOME_DIRECTORY}/examples/platforms --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/app-bittorrent app-bittorrent.tesh)
endif()
