cmake_minimum_required(VERSION 2.6)

if(enable_java)
  add_custom_target(java_bittorrent ALL
    COMMAND ${JAVA_COMPILE} -cp ${CMAKE_BINARY_DIR}/simgrid.jar
                            -d ${CMAKE_CURRENT_BINARY_DIR}/..
                            ${CMAKE_CURRENT_SOURCE_DIR}/*.java)
  add_dependencies(java_bittorrent SG_java_jar)
endif()

set(tesh_files
  ${tesh_files}
  examples/bittorrent/bittorrent.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  examples/bittorrent/MessageTask.java
  examples/bittorrent/Peer.java
  examples/bittorrent/TrackerTask.java
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

