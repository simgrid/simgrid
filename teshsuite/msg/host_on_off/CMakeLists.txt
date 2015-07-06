cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

foreach(x host_on_off host_on_off_wait host_on_off_recv)
  add_executable(${x} ${x}.c)
  target_link_libraries(${x} simgrid)
  set(tesh_files
    ${tesh_files}
    ${CMAKE_CURRENT_SOURCE_DIR}/${x}.tesh
    PARENT_SCOPE
    )
endforeach()

set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off_d.xml
  PARENT_SCOPE
  )
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off.c
  ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off_simple.c
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
