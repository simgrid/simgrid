cmake_minimum_required(VERSION 2.6)

set(example java_reservation_surf_plugin)
set(sources
   ${CMAKE_CURRENT_SOURCE_DIR}/TestPlugin.java
   ${CMAKE_CURRENT_SOURCE_DIR}/ReservationPlugin.java
   ${CMAKE_CURRENT_SOURCE_DIR}/Sender.java
   ${CMAKE_CURRENT_SOURCE_DIR}/Receiver.java
  )

if(enable_java)
  add_custom_command(
    COMMENT "Building ${example}..."
    OUTPUT ${example}_compiled
    DEPENDS ${sources} simgrid-java_jar ${SIMGRID_JAR}
    COMMAND ${JAVA_COMPILE} -classpath ${SIMGRID_JAR}
                             -d ${CMAKE_CURRENT_BINARY_DIR}/.. ${sources}
    COMMAND ${CMAKE_COMMAND} -E remove ${example}_compiled
    COMMAND ${CMAKE_COMMAND} -E touch ${example}_compiled
  )
  add_custom_target(${example} ALL DEPENDS ${example}_compiled)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/reservation_surf_plugin.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/reservationSurfPluginPlatform.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/reservationSurfPluginDeployment.xml
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
