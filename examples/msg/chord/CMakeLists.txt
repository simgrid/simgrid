add_executable       (chord chord.c)
target_link_libraries(chord simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/chord.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/chord_crosstraffic.tesh
  PARENT_SCOPE)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/chord.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/chord10.xml
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/chord.c
  PARENT_SCOPE)
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/generate.py
  ${CMAKE_CURRENT_SOURCE_DIR}/run_chord
  PARENT_SCOPE)
