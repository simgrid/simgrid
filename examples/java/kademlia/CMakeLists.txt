cmake_minimum_required(VERSION 2.6)

set(sources
  ${CMAKE_CURRENT_SOURCE_DIR}/Answer.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Bucket.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Common.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Contact.java
  ${CMAKE_CURRENT_SOURCE_DIR}/FindNodeAnswerTask.java
  ${CMAKE_CURRENT_SOURCE_DIR}/FindNodeTask.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Kademlia.java
  ${CMAKE_CURRENT_SOURCE_DIR}/KademliaTask.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Node.java
  ${CMAKE_CURRENT_SOURCE_DIR}/PingAnswerTask.java
  ${CMAKE_CURRENT_SOURCE_DIR}/PingTask.java
  ${CMAKE_CURRENT_SOURCE_DIR}/RoutingTable.java
  )

if(enable_java)
  add_custom_target(java_kademlia ALL
    COMMENT "Building java_kademlia..."
    COMMAND ${JAVA_COMPILE} -cp ${CMAKE_BINARY_DIR}/simgrid.jar
                            -d ${CMAKE_CURRENT_BINARY_DIR}/.. ${sources})
  add_dependencies(java_kademlia SG_java_jar)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/kademlia.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/kademlia.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${sources}
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  PARENT_SCOPE
  )
