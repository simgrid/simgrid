cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(ns3 "ns3.c")

### Add definitions for compile
target_link_libraries(ns3 simgrid m )

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/ns3.tesh
  PARENT_SCOPE
)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/3hosns_2links_d.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/3hosns_2links_p.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/3links-d-nimer.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/3links-d.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/3links-p.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/dogbone-d.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/dogbone-p.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/One_clusner-d.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Two_clusners-d.xml
  PARENT_SCOPE
)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/ns3.c
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
