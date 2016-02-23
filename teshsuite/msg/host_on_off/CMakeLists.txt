foreach(x host_on_off host_on_off_wait host_on_off_recv)
  add_executable       (${x} ${x}.c)
  target_link_libraries(${x} simgrid)
endforeach()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off_wait.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off_recv.tesh
  PARENT_SCOPE)
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off.c
  ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off_wait.c
  ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off_recv.c
  PARENT_SCOPE)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off_d.xml
  PARENT_SCOPE)
