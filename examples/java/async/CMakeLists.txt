cmake_minimum_required(VERSION 2.6)

set(example java_async)
set(sources
  ${CMAKE_CURRENT_SOURCE_DIR}/AsyncTest.java
  ${CMAKE_CURRENT_SOURCE_DIR}/FinalizeTask.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Forwarder.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Master.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Slave.java
  )

if(enable_java)
  add_custom_command(
    COMMENT "Building ${example}..."
    OUTPUT ${example}_compiled
    DEPENDS ${sources} ${SIMGRID_JAR}
    COMMAND ${JAVA_COMPILE} -cp ${SIMGRID_JAR}
                             -d ${CMAKE_CURRENT_BINARY_DIR}/.. ${sources}
    COMMAND ${CMAKE_COMMAND} -E touch ${example}_compiled
  )
  add_custom_target(${example} ALL DEPENDS ${example}_compiled)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/async.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/asyncDeployment.xml
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
