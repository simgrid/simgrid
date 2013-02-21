cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(gtnets gtnets.c)

### Add definitions for compile
target_link_libraries(gtnets simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/gtnets-crosstraffic.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/gtnets-onelink-gtnets.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/tracing-gtnets-dogbone-gtnets.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/tracing-gtnets-onelink-lv08.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/gtnets-dogbone-gtnets.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/gtnets-onelink-lv08.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/tracing-gtnets-dogbone-lv08.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/tracing-gtnets-waxman.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/gtnets-dogbone-lv08.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/gtnets-waxman.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/tracing-gtnets-onelink-gtnets.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/crosstraffic-d.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/crosstraffic-p.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/dogbone-d.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/dogbone-p.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/onelink-d.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/onelink-p.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/waxman-d.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/waxman-p.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/gtnets.c
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
