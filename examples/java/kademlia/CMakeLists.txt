set(example java_kademlia)
set(sources ${CMAKE_CURRENT_SOURCE_DIR}/Kademlia.java      ${CMAKE_CURRENT_SOURCE_DIR}/Answer.java
            ${CMAKE_CURRENT_SOURCE_DIR}/Bucket.java        ${CMAKE_CURRENT_SOURCE_DIR}/Common.java
            ${CMAKE_CURRENT_SOURCE_DIR}/Contact.java       ${CMAKE_CURRENT_SOURCE_DIR}/FindNodeAnswerTask.java
            ${CMAKE_CURRENT_SOURCE_DIR}/FindNodeTask.java  ${CMAKE_CURRENT_SOURCE_DIR}/KademliaTask.java
            ${CMAKE_CURRENT_SOURCE_DIR}/Node.java          ${CMAKE_CURRENT_SOURCE_DIR}/PingAnswerTask.java
            ${CMAKE_CURRENT_SOURCE_DIR}/PingTask.java      ${CMAKE_CURRENT_SOURCE_DIR}/RoutingTable.java)

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
  ADD_TESH(java-kademlia --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/kademlia/kademlia.tesh)
endif()

set(tesh_files    ${tesh_files}    ${CMAKE_CURRENT_SOURCE_DIR}/kademlia.tesh  PARENT_SCOPE)
set(xml_files     ${xml_files}     ${CMAKE_CURRENT_SOURCE_DIR}/kademlia.xml   PARENT_SCOPE)
set(examples_src  ${examples_src}  ${sources}                                 PARENT_SCOPE)
