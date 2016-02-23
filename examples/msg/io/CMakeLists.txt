add_executable       (file        file.c)
target_link_libraries(file        simgrid)
add_executable       (file_unlink file_unlink.c)
target_link_libraries(file_unlink simgrid)
add_executable       (storage     storage.c)
target_link_libraries(storage     simgrid)
add_executable       (remote      remote.c)
target_link_libraries(remote      simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/io.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/storage.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/remote.tesh
  PARENT_SCOPE)
set(xml_files
  ${xml_files}
  examples/msg/io/remote_io.deployment.xml
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/file.c
  ${CMAKE_CURRENT_SOURCE_DIR}/file_unlink.c
  ${CMAKE_CURRENT_SOURCE_DIR}/storage.c
  ${CMAKE_CURRENT_SOURCE_DIR}/remote.c
  PARENT_SCOPE)
