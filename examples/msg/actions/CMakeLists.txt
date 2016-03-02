add_executable       (actions         actions.c)
target_link_libraries(actions         simgrid)
add_executable       (storage_actions storage_actions.c)
target_link_libraries(storage_actions simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/actions.tesh
  PARENT_SCOPE)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_split.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/storage_deployment.xml
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/actions.c
  ${CMAKE_CURRENT_SOURCE_DIR}/storage_actions.c
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/mpi_actions_shared.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/mpi_actions_split_p0.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/mpi_actions_split_p1.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/storage_actions.txt
  PARENT_SCOPE)
