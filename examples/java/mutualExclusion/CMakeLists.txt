cmake_minimum_required(VERSION 2.6)

set(sources
  ${CMAKE_CURRENT_SOURCE_DIR}/Coordinator.java
  ${CMAKE_CURRENT_SOURCE_DIR}/GrantTask.java
  ${CMAKE_CURRENT_SOURCE_DIR}/MutexCentral.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Node.java
  ${CMAKE_CURRENT_SOURCE_DIR}/ReleaseTask.java
  ${CMAKE_CURRENT_SOURCE_DIR}/RequestTask.java
  )

if(enable_java)
  add_custom_target(java_mutualExclusion ALL
    COMMENT "Building java_mutualExclusion..."
    COMMAND ${JAVA_COMPILE} -cp ${CMAKE_BINARY_DIR}/simgrid.jar
                            -d ${CMAKE_CURRENT_BINARY_DIR}/.. ${sources})
  add_dependencies(java_mutualExclusion SG_java_jar)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/mutualexclusion.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/mutex_centralized_deployment.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/ring3.xml
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
