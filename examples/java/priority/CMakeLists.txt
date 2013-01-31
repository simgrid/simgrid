cmake_minimum_required(VERSION 2.6)

set(sources
  ${CMAKE_CURRENT_SOURCE_DIR}/Priority.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Test.java
  )

if(enable_java)
  add_custom_target(java_priority ALL
    COMMENT "Building java_priority..."
    COMMAND ${JAVA_COMPILE} -cp ${CMAKE_BINARY_DIR}/simgrid.jar
                            -d ${CMAKE_CURRENT_BINARY_DIR}/.. ${sources})
  add_dependencies(java_priority SG_java_jar)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/priority.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/priorityDeployment.xml
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
