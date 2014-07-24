cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(host_on_off_processes host_on_off_processes.c)

### Add definitions for compile
target_link_libraries(host_on_off_processes simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off_processes.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off_processes_d.xml
  PARENT_SCOPE
  )
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off_processes.c
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
