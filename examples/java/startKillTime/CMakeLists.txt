cmake_minimum_required(VERSION 2.6)

set(example java_startKillTime)
set(sources
  ${CMAKE_CURRENT_SOURCE_DIR}/Master.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Slave.java
  ${CMAKE_CURRENT_SOURCE_DIR}/StartKillTime.java
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
  ${CMAKE_CURRENT_SOURCE_DIR}/startKillTime.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_kill.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_start.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_start_kill.xml
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
