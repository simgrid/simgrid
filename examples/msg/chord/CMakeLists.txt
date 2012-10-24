cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(chord chord.c)
add_executable(chord_before_dsend chord_before_dsend.c)

### Add definitions for compile
target_link_libraries(chord simgrid )
target_link_libraries(chord_before_dsend simgrid )

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/chord.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/chord_crosstraffic.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/chord.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/chord10.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/chord100.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/chord90.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/chord.c
  ${CMAKE_CURRENT_SOURCE_DIR}/chord_before_dsend.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/generate.py
  ${CMAKE_CURRENT_SOURCE_DIR}/run_chord
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  PARENT_SCOPE
  )
