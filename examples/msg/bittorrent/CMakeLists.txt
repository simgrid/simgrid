cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(bittorrent
  "bittorrent.c" "messages.c" "peer.c" "tracker.c" "connection.c")

### Add definitions for compile
target_link_libraries(bittorrent simgrid )

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
  ${CMAKE_CURRENT_SOURCE_DIR}/bittorrent.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bittorrent.h
  ${CMAKE_CURRENT_SOURCE_DIR}/connection.c
  ${CMAKE_CURRENT_SOURCE_DIR}/connection.h
  ${CMAKE_CURRENT_SOURCE_DIR}/messages.c
  ${CMAKE_CURRENT_SOURCE_DIR}/messages.h
  ${CMAKE_CURRENT_SOURCE_DIR}/peer.c
  ${CMAKE_CURRENT_SOURCE_DIR}/peer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/tracker.c
  ${CMAKE_CURRENT_SOURCE_DIR}/tracker.h
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/generate.py
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  PARENT_SCOPE
  )
