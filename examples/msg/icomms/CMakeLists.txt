cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(peer peer.c)
target_link_libraries(peer simgrid)

add_executable(peer2 peer2.c)
target_link_libraries(peer2 simgrid)

add_executable(peer3 peer3.c)
target_link_libraries(peer3 simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/peer.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_peer.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_peer01.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_peer02.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_peer03.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_peer04.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_peer05.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/small_platform.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/peer.c
  ${CMAKE_CURRENT_SOURCE_DIR}/peer2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/peer3.c
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
