set(app-bittorrent_files        Main  Common  Connection MessageTask Peer  Tracker TrackerTask)
set(app-centralizedmutex_files  Main  Coordinator  GrantTask Node ReleaseTask  RequestTask)
set(app-masterworker_files      Main  Master Worker)
set(app-pingpong_files          Main  PingPongTask Receiver Sender)
set(app-tokenring_files         Main  RelayRunner)
set(async-waitall_files         Main  Receiver Sender)
set(async-yield_files           Main  Yielder)
set(async-dsend_files           Main  Receiver Sender)
set(cloud-masterworker_files    Main  Master Worker)
set(cloud-migration_files       Main  Daemon Test XVM)
set(dht-chord_files             Main  ChordTask  Common FindSuccessorAnswerTask  FindSuccessorTask
                                      GetPredecessorAnswerTask GetPredecessorTask Node  NotifyTask)
set(dht-kademlia_files          Main  Answer  Bucket  Common Contact FindNodeAnswerTask  FindNodeTask
                                      KademliaTask  Node  RoutingTable)
set(trace-pingpong_files        Main  PingPongTask Receiver Sender)
set(energy-consumption_files    Main  EnergyConsumer)
set(energy-pstate_files         Main  PstateRunner)
set(energy-vm_files             Main  EnergyVMRunner)
set(process-kill_files          Main  Killer  Victim)
set(process-migration_files     Main  Emigrant  Policeman)
set(process-startkilltime_files Main  Sleeper)
set(process-suspend_files       Main  DreamMaster  LazyGuy)
set(task-priority_files         Main  Test)
set(hostload_files              Main  LoadRunner)

if(enable_java)
  add_custom_target(java-all COMMENT "Building all Java examples...")
  add_dependencies(tests java-all)
  add_dependencies(java-all simgrid-java) # useful when the libs are not included in the jar
endif()

foreach (example app-bittorrent app-centralizedmutex app-masterworker app-pingpong app-tokenring async-yield async-waitall async-dsend
         cloud-migration cloud-masterworker dht-chord dht-kademlia energy-consumption energy-pstate energy-vm hostload
         process-kill process-migration process-startkilltime process-suspend task-priority trace-pingpong)
  string (REPLACE "-" "/" example_dir ${example})
  set (srcdir ${CMAKE_CURRENT_SOURCE_DIR}/${example_dir})
  foreach (filename ${${example}_files} )
    set( ${example}_sources "${${example}_sources}" "${srcdir}/${filename}.java")
  endforeach()

  if(enable_java)
    add_jar(java-${example} SOURCES ${${example}_sources}
            INCLUDE_JARS simgrid-java_jar
	    OUTPUT_NAME ${example}
	    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${example_dir})
    add_dependencies(java-all java-${example})
    
    string (REPLACE "-" "/" example_dir ${example})
    ADD_TESH(java-${example}  --setenv javacmd=${Java_JAVA_EXECUTABLE} --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/deprecated/java --setenv LD_LIBRARY_PATH=${TESH_LIBRARY_PATH} --setenv classpath=""${TESH_CLASSPATH}${TESH_CLASSPATH_SEPARATOR}${example}.jar"" --cd ${CMAKE_BINARY_DIR}/examples/deprecated/java/${example_dir} ${CMAKE_HOME_DIRECTORY}/examples/deprecated/java/${example_dir}/${example}.tesh)
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
