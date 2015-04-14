cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(dax_test dax_test.c)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(dax_test simgrid pthread )
else()
  target_link_libraries(dax_test simgrid)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/simple_dax_with_cycle.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/smalldax.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/Montage_50.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/simple_dax_with_cycle.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/smalldax.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/dax_test.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/generate_forkjoin.pl
  ${CMAKE_CURRENT_SOURCE_DIR}/generate_strassen.pl
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  PARENT_SCOPE
  )
