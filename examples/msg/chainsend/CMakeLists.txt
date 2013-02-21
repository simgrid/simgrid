cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(chainsend chainsend.c iterator.c common.c messages.c broadcaster.c peer.c)

### Add definitions for compile
target_link_libraries(chainsend simgrid )

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/chainsend.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_tesh.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_chainsend.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/iterator.h
  ${CMAKE_CURRENT_SOURCE_DIR}/iterator.c
  ${CMAKE_CURRENT_SOURCE_DIR}/common.h
  ${CMAKE_CURRENT_SOURCE_DIR}/common.c
  ${CMAKE_CURRENT_SOURCE_DIR}/messages.h
  ${CMAKE_CURRENT_SOURCE_DIR}/messages.c
  ${CMAKE_CURRENT_SOURCE_DIR}/broadcaster.h
  ${CMAKE_CURRENT_SOURCE_DIR}/broadcaster.c
  ${CMAKE_CURRENT_SOURCE_DIR}/peer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/peer.c
  ${CMAKE_CURRENT_SOURCE_DIR}/chainsend.c
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
