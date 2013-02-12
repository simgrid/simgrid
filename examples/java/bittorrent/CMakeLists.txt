cmake_minimum_required(VERSION 2.6)

set(example java_bittorrent)
set(sources
  ${CMAKE_CURRENT_SOURCE_DIR}/Bittorrent.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Common.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Connection.java
  ${CMAKE_CURRENT_SOURCE_DIR}/MessageTask.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Peer.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Tracker.java
  ${CMAKE_CURRENT_SOURCE_DIR}/TrackerTask.java
  )

if(enable_java)
  add_custom_command(
    COMMENT "Building ${example}..."
    OUTPUT ${example}_compiled
    DEPENDS ${sources} SG_java_jar ${SIMGRID_JAR}
    COMMAND ${JAVA_COMPILE} -cp ${SIMGRID_JAR}
                             -d ${CMAKE_CURRENT_BINARY_DIR}/.. ${sources}
    COMMAND ${CMAKE_COMMAND} -E remove ${example}_compiled
    COMMAND ${CMAKE_COMMAND} -E touch ${example}_compiled
  )
  add_custom_target(${example} ALL DEPENDS ${example}_compiled)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/bittorrent.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/bittorrent.xml
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
  ${CMAKE_CURRENT_SOURCE_DIR}/generate.py
  PARENT_SCOPE
  )
