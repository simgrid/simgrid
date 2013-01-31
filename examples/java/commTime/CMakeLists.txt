cmake_minimum_required(VERSION 2.6)

set(sources
  ${CMAKE_CURRENT_SOURCE_DIR}/CommTimeTest.java
  ${CMAKE_CURRENT_SOURCE_DIR}/FinalizeTask.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Master.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Slave.java
  )

if(enable_java)
  add_custom_target(java_commTime ALL
    COMMENT "Building java_commTime..."
    COMMAND ${JAVA_COMPILE} -cp ${CMAKE_BINARY_DIR}/simgrid.jar
                            -d ${CMAKE_CURRENT_BINARY_DIR}/.. ${sources})
  add_dependencies(java_commTime SG_java_jar)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/commtime.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/commTimeDeployment.xml
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
  ${CMAKE_CURRENT_SOURCE_DIR}/README
  PARENT_SCOPE
  )
