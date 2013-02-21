cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
add_executable(actions actions.c)

### Add definitions for compile
target_link_libraries(actions simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/actions.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_split.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/homogeneous_3_hosts.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/actions.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/actions_allReduce.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/actions_barrier.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/actions_bcast.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/actions_reduce.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/actions_split_p0.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/actions_split_p1.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/actions.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/actions_with_isend.txt

  PARENT_SCOPE
  )
