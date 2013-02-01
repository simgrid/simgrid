cmake_minimum_required(VERSION 2.6)

set(example java_chord)
set(sources
  ${CMAKE_CURRENT_SOURCE_DIR}/Chord.java
  ${CMAKE_CURRENT_SOURCE_DIR}/ChordTask.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Common.java
  ${CMAKE_CURRENT_SOURCE_DIR}/FindSuccessorAnswerTask.java
  ${CMAKE_CURRENT_SOURCE_DIR}/FindSuccessorTask.java
  ${CMAKE_CURRENT_SOURCE_DIR}/GetPredecessorAnswerTask.java
  ${CMAKE_CURRENT_SOURCE_DIR}/GetPredecessorTask.java
  ${CMAKE_CURRENT_SOURCE_DIR}/Node.java
  ${CMAKE_CURRENT_SOURCE_DIR}/NotifyTask.java
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
  ${CMAKE_CURRENT_SOURCE_DIR}/chord.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/chord.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/chord10.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/chord100.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/chord1000.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/chord10000.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/chord90.xml
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
