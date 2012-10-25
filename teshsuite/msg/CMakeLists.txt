cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(get_sender ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/get_sender.c)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(get_sender simgrid m pthread )
else()
  target_link_libraries(get_sender simgrid)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/get_sender.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/get_sender.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/get_sender_d.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/get_sender_p.xml
  PARENT_SCOPE
  )
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/get_sender.c
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
