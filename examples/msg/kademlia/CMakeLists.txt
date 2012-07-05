cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(kademlia "kademlia.c" "node.c" "routing_table.c" 
"task.c" "answer.c")
### Add definitions for compile
target_link_libraries(kademlia simgrid )

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/kademlia.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/kademlia.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/answer.c
  ${CMAKE_CURRENT_SOURCE_DIR}/answer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/common.h
  ${CMAKE_CURRENT_SOURCE_DIR}/kademlia.c
  ${CMAKE_CURRENT_SOURCE_DIR}/kademlia.h
  ${CMAKE_CURRENT_SOURCE_DIR}/node.c
  ${CMAKE_CURRENT_SOURCE_DIR}/node.h
  ${CMAKE_CURRENT_SOURCE_DIR}/routing_table.c
  ${CMAKE_CURRENT_SOURCE_DIR}/routing_table.h
  ${CMAKE_CURRENT_SOURCE_DIR}/task.c
  ${CMAKE_CURRENT_SOURCE_DIR}/task.h
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
