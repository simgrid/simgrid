set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(storage_basic storage_basic.c)
add_executable(concurrent_rw concurrent_rw.c)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(storage_basic simgrid m pthread )
  target_link_libraries(concurrent_rw simgrid m pthread )
else()
  target_link_libraries(storage_basic simgrid)
  target_link_libraries(concurrent_rw simgrid)
endif()

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
