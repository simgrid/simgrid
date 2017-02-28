set(app_bittorrent_files        Main  Common  Connection MessageTask Peer  Tracker TrackerTask)
set(app_centralizedmutex_files  Main  Coordinator  GrantTask Node ReleaseTask  RequestTask)
set(app_masterworker_files      Main  Master Worker) 
set(app_pingpong_files          Main  PingPongTask Receiver Sender) 
set(app_tokenring_files         Main RelayRunner)
set(async_waitall_files         Main  Receiver Sender)
set(async_yield_files           Main  Yielder)
set(async_dsend_files           Main  Receiver Sender)
set(cloud_masterworker_files    Main  Master Worker)
set(cloud_migration_files       Main  Daemon Test TestHostOnOff XVM)
set(dht_chord_files             Main  ChordTask  Common FindSuccessorAnswerTask  FindSuccessorTask
                                      GetPredecessorAnswerTask GetPredecessorTask Node  NotifyTask)
set(dht_kademlia_files          Main  Answer  Bucket  Common Contact FindNodeAnswerTask  FindNodeTask
                                      KademliaTask  Node  PingAnswerTask PingTask  RoutingTable)
set(trace_pingpong_files        Main  PingPongTask Receiver Sender) 
set(energy_consumption_files    Main  EnergyConsumer)
set(energy_pstate_files         Main  PstateRunner)
set(energy_vm_files             Main  EnergyVMRunner)
set(io_file_files               Main  Node)
set(io_storage_files            Main  Client)
set(process_kill_files          Main  Killer  Victim)
set(process_migration_files     Main  Emigrant  Policeman)
set(process_startkilltime_files Main Sleeper)
set(process_suspend_files       Main  DreamMaster  LazyGuy)
set(task_priority_files         Main  Test)


foreach (example app_bittorrent app_centralizedmutex app_masterworker app_pingpong app_tokenring async_yield async_waitall async_dsend
         cloud_migration cloud_masterworker dht_chord dht_kademlia energy_consumption energy_pstate energy_vm io_file io_storage 
         process_kill process_migration process_startkilltime process_suspend task_priority trace_pingpong)
  string (REPLACE "_" "/" example_dir ${example})
  set (srcdir ${CMAKE_CURRENT_SOURCE_DIR}/${example_dir})
  foreach (filename ${${example}_files} )
    set( ${example}_sources "${${example}_sources}" "${srcdir}/${filename}.java")
  endforeach()

  if(enable_java)
    add_custom_command(
      COMMENT "Building java_${example}..."
      OUTPUT ${example_dir}/java_${example}_compiled
      DEPENDS ${example_sources} simgrid-java_jar ${SIMGRID_JAR}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${example_dir}
      COMMAND ${JAVA_COMPILE} -classpath ${SIMGRID_JAR} -d ${CMAKE_CURRENT_BINARY_DIR} ${${example}_sources}
      COMMAND ${CMAKE_COMMAND} -E remove ${example_dir}/java_${example}_compiled
      COMMAND ${CMAKE_COMMAND} -E touch ${example_dir}/java_${example}_compiled
    )
    add_custom_target(${example} ALL DEPENDS ${example_dir}/java_${example}_compiled)
  endif()
  set(examples_src  ${examples_src}  ${${example}_sources})
  set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/${example_dir}/${example}.tesh)
endforeach()

set(examples_src  ${examples_src}                                                                          PARENT_SCOPE)
set(tesh_files    ${tesh_files}                                                                            PARENT_SCOPE)
set(bin_files     ${bin_files}     ${CMAKE_CURRENT_SOURCE_DIR}/app/bittorrent/generate.py                  PARENT_SCOPE)
set(txt_files     ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/app/masterworker/README
                                   ${CMAKE_CURRENT_SOURCE_DIR}/cloud/migration/README                      PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/app/bittorrent/bittorrent.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/app/centralizedmutex/centralizedmutex.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/app/masterworker/masterworker.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/dht/chord/chord.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/dht/kademlia/kademlia.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/process/startkilltime/startkilltime.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/task/priority/priority.xml                  PARENT_SCOPE)

if(enable_java)
  foreach (example app_bittorrent app_centralizedmutex app_masterworker app_pingpong app_tokenring async_yield async_waitall async_dsend
           cloud_migration cloud_masterworker dht_chord dht_kademlia energy_consumption energy_pstate energy_vm io_file io_storage 
           process_kill process_migration process_startkilltime process_suspend task_priority trace_pingpong)
    string (REPLACE "_" "/" example_dir ${example})
    ADD_TESH(java-${example}  --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/${example_dir}/${example}.tesh)
  endforeach()
endif()
