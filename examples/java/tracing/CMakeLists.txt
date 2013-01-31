cmake_minimum_required(VERSION 2.6)

set(sources
  ${CMAKE_CURRENT_SOURCE_DIR}/PingPongTask.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Receiver.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Sender.java
  ${CMAKE_CURRENT_SOURCE_DIR}/TracingTest.java
  )

if(enable_java AND HAVE_TRACING)
  add_custom_target(java_tracing ALL
    COMMENT "Building java_tracing..."
    COMMAND ${JAVA_COMPILE} -cp ${CMAKE_BINARY_DIR}/simgrid.jar
                            -d ${CMAKE_CURRENT_BINARY_DIR}/.. ${sources})
  add_dependencies(java_tracing SG_java_jar)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/tracingPingPong.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/tracingPingPongDeployment.xml
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
