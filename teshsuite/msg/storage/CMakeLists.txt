cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(storage storage.c)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(storage simgrid m pthread )
else()
  target_link_libraries(storage simgrid)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/storage.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/platform.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment.xml
  PARENT_SCOPE
  )
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/storage.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/storage_content_c.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/storage_content_s1.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/storage_content_s2.txt
  PARENT_SCOPE
  )
