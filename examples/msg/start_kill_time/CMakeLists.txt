cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(sk_time "sk_time.c")

### Add definitions for compile
if(WIN32)
  target_link_libraries(sk_time simgrid )
else()
  target_link_libraries(sk_time simgrid )
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/start_kill_time.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_kill.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_start_kill.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_start.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/sk_time.c
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
