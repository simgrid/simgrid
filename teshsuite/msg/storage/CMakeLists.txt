add_executable       (storage_basic storage_basic.c)
target_link_libraries(storage_basic simgrid)
add_executable       (concurrent_rw concurrent_rw.c)
target_link_libraries(concurrent_rw simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/storage_basic.tesh
  PARENT_SCOPE)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment.xml
  PARENT_SCOPE)
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/storage_basic.c
  ${CMAKE_CURRENT_SOURCE_DIR}/concurrent_rw.c
  PARENT_SCOPE)
