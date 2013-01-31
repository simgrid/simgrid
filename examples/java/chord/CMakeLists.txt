cmake_minimum_required(VERSION 2.6)

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
  add_custom_target(java_chord ALL
    COMMENT "Building java_chord..."
    COMMAND ${JAVA_COMPILE} -cp ${CMAKE_BINARY_DIR}/simgrid.jar
                            -d ${CMAKE_CURRENT_BINARY_DIR}/.. ${sources})
  add_dependencies(java_chord SG_java_jar)
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
