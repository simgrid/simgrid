set (srcdir ${CMAKE_CURRENT_SOURCE_DIR}/app/bittorrent)
set(app_bittorrent_sources    ${srcdir}/Main.java  ${srcdir}/Common.java  ${srcdir}/Connection.java
                              ${srcdir}/MessageTask.java ${srcdir}/Peer.java  ${srcdir}/Tracker.java
                              ${srcdir}/TrackerTask.java)

set (srcdir ${CMAKE_CURRENT_SOURCE_DIR}/app/masterworker)
set(app_masterworker_sources   ${srcdir}/Main.java  ${srcdir}/Master.java ${srcdir}/Worker.java) 

set (srcdir ${CMAKE_CURRENT_SOURCE_DIR}/cloud/masterworker)
set(cloud_masterworker_sources ${srcdir}/Main.java  ${srcdir}/Master.java ${srcdir}/Worker.java)

set (srcdir ${CMAKE_CURRENT_SOURCE_DIR}/cloud/migration)
set(cloud_migration_sources    ${srcdir}/Main.java  ${srcdir}/Daemon.java ${srcdir}/Test.java  
                               ${srcdir}/TestHostOnOff.java ${srcdir}/XVM.java)
                               
set (srcdir ${CMAKE_CURRENT_SOURCE_DIR}/dht/chord)
set(dht_chord_sources          ${srcdir}/Main.java  ${srcdir}/ChordTask.java  ${srcdir}/Common.java
                               ${srcdir}/FindSuccessorAnswerTask.java  ${srcdir}/FindSuccessorTask.java
                               ${srcdir}/GetPredecessorAnswerTask.java ${srcdir}/GetPredecessorTask.java
                               ${srcdir}/Node.java  ${srcdir}/NotifyTask.java)

set (srcdir ${CMAKE_CURRENT_SOURCE_DIR}/dht/kademlia)
set(dht_kademlia_sources       ${srcdir}/Main.java  ${srcdir}/Answer.java  ${srcdir}/Bucket.java  ${srcdir}/Common.java
                               ${srcdir}/Contact.java  ${srcdir}/FindNodeAnswerTask.java  ${srcdir}/FindNodeTask.java
                               ${srcdir}/KademliaTask.java  ${srcdir}/Node.java  ${srcdir}/PingAnswerTask.java
                               ${srcdir}/PingTask.java  ${srcdir}/RoutingTable.java)

set (srcdir ${CMAKE_CURRENT_SOURCE_DIR}/energy/consumption)
set(energy_consumption_sources ${srcdir}/Main.java  ${srcdir}/EnergyConsumer.java)

set (srcdir ${CMAKE_CURRENT_SOURCE_DIR}/energy/vm)
set(energy_vm_sources          ${srcdir}/Main.java  ${srcdir}/EnergyVMRunner.java)

set (srcdir ${CMAKE_CURRENT_SOURCE_DIR}/io/file)
set(io_file_sources            ${srcdir}/Main.java  ${srcdir}/Node.java)

set (srcdir ${CMAKE_CURRENT_SOURCE_DIR}/io/storage)
set(io_storage_sources         ${srcdir}/Main.java  ${srcdir}/Client.java)

set (srcdir ${CMAKE_CURRENT_SOURCE_DIR}/process/kill)
set(process_kill_sources       ${srcdir}/Main.java  ${srcdir}/Killer.java  ${srcdir}/Victim.java)

set (srcdir ${CMAKE_CURRENT_SOURCE_DIR}/process/migration)
set(process_migration_sources  ${srcdir}/Main.java  ${srcdir}/Emigrant.java  ${srcdir}/Policeman.java)

set (srcdir ${CMAKE_CURRENT_SOURCE_DIR}/process/startkilltime)
set(process_startkilltime_sources ${srcdir}/Main.java ${srcdir}/Sleeper.java)

set (srcdir ${CMAKE_CURRENT_SOURCE_DIR}/process/suspend)
set(process_suspend_sources    ${srcdir}/Main.java  ${srcdir}/DreamMaster.java  ${srcdir}/LazyGuy.java)

foreach (example app_bittorrent app_masterworker cloud_migration cloud_masterworker dht_chord dht_kademlia 
         energy_consumption energy_vm io_file io_storage process_kill process_migration process_startkilltime 
         process_suspend)
  string (REPLACE "_" "/" example_dir ${example})
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

set(examples_src  ${examples_src}                                                          PARENT_SCOPE)
set(tesh_files    ${tesh_files}                                                            PARENT_SCOPE)
set(bin_files     ${bin_files}     ${CMAKE_CURRENT_SOURCE_DIR}/app/bittorrent/generate.py  PARENT_SCOPE)
set(txt_files     ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/app/masterworker/README
                                   ${CMAKE_CURRENT_SOURCE_DIR}/cloud/migration/README      PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/app/bittorrent/bittorrent.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/app/masterworker/masterworker.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/dht/chord/chord.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/dht/kademlia/kademlia.xml
                                   ${CMAKE_CURRENT_SOURCE_DIR}/process/startkilltime/startkilltime.xml   PARENT_SCOPE)

if(enable_java)
  foreach (example app_bittorrent app_masterworker cloud_migration cloud_masterworker dht_chord dht_kademlia 
         energy_consumption energy_vm io_file io_storage process_kill process_migration process_startkilltime 
         process_suspend)
    string (REPLACE "_" "/" example_dir ${example})
    ADD_TESH(java-${example}  --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/${example_dir}/${example}.tesh)
  endforeach()
endif()
