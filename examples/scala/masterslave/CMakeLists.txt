cmake_minimum_required(VERSION 2.6)

set(example scala_masterslave)
set(sources
  ${CMAKE_CURRENT_SOURCE_DIR}/FinalizeTask.scala
  ${CMAKE_CURRENT_SOURCE_DIR}/Forwarder.scala
  ${CMAKE_CURRENT_SOURCE_DIR}/Master.scala
  ${CMAKE_CURRENT_SOURCE_DIR}/Masterslave.scala
  ${CMAKE_CURRENT_SOURCE_DIR}/Slave.scala
  )

if(enable_scala)
  add_custom_command(
    COMMENT "Building ${example}..."
    OUTPUT ${example}_compiled
    DEPENDS ${sources} SG_java_jar ${SIMGRID_JAR}
    COMMAND ${SCALA_COMPILE} -cp ${SIMGRID_JAR}
                             -d ${CMAKE_CURRENT_BINARY_DIR}/.. ${sources}
    COMMAND ${CMAKE_COMMAND} -E remove ${example}_compiled
    COMMAND ${CMAKE_COMMAND} -E touch ${example}_compiled
  )
  add_custom_target(${example} ALL DEPENDS ${example}_compiled)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deploy.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslaveDeployment.xml
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
