set(example java_bittorrent)
set(sources ${CMAKE_CURRENT_SOURCE_DIR}/Bittorrent.java   ${CMAKE_CURRENT_SOURCE_DIR}/Common.java
            ${CMAKE_CURRENT_SOURCE_DIR}/Connection.java   ${CMAKE_CURRENT_SOURCE_DIR}/MessageTask.java
            ${CMAKE_CURRENT_SOURCE_DIR}/Peer.java         ${CMAKE_CURRENT_SOURCE_DIR}/Tracker.java
            ${CMAKE_CURRENT_SOURCE_DIR}/TrackerTask.java)

if(enable_java)
  add_custom_command(
    COMMENT "Building ${example}..."
    OUTPUT ${example}_compiled
    DEPENDS ${sources} simgrid-java_jar ${SIMGRID_JAR}
    COMMAND ${JAVA_COMPILE} -classpath ${SIMGRID_JAR} -d ${CMAKE_CURRENT_BINARY_DIR}/.. ${sources}
    COMMAND ${CMAKE_COMMAND} -E remove ${example}_compiled
    COMMAND ${CMAKE_COMMAND} -E touch ${example}_compiled
  )
  add_custom_target(${example} ALL DEPENDS ${example}_compiled)
  ADD_TESH(java-bittorrent --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/bittorrent/bittorrent.tesh)
endif()


set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/bittorrent.tesh  PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/bittorrent.xml   PARENT_SCOPE)
set(examples_src  ${examples_src}  ${sources}                                   PARENT_SCOPE)
set(txt_files     ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/generate.py      PARENT_SCOPE)
