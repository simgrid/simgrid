cmake_minimum_required(VERSION 2.6)

set_source_files_properties(
  ${CMAKE_CURRENT_BINARY_DIR}/_pmm_simulator.c
  ${CMAKE_CURRENT_BINARY_DIR}/_pmm_slave.c
  ${CMAKE_CURRENT_BINARY_DIR}/_pmm_master.c
  PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
add_executable(pmm_simulator ${CMAKE_CURRENT_BINARY_DIR}/_pmm_simulator.c ${CMAKE_CURRENT_SOURCE_DIR}/pmm.c)
add_executable(pmm_slave     ${CMAKE_CURRENT_BINARY_DIR}/_pmm_slave.c ${CMAKE_CURRENT_SOURCE_DIR}/pmm.c)
add_executable(pmm_master    ${CMAKE_CURRENT_BINARY_DIR}/_pmm_master.c ${CMAKE_CURRENT_SOURCE_DIR}/pmm.c)

add_custom_command(
  OUTPUT
  ${CMAKE_CURRENT_BINARY_DIR}/_pmm_simulator.c
  ${CMAKE_CURRENT_BINARY_DIR}/_pmm_slave.c
  ${CMAKE_CURRENT_BINARY_DIR}/_pmm_master.c
  DEPENDS gras_stub_generator ${CMAKE_CURRENT_SOURCE_DIR}/pmm.xml
  COMMAND ${CMAKE_BINARY_DIR}/bin/gras_stub_generator pmm ${CMAKE_CURRENT_SOURCE_DIR}/pmm.xml
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(pmm_simulator simgrid pthread m )
  target_link_libraries(pmm_slave gras pthread m )
  target_link_libraries(pmm_master gras pthread m )
else()
  target_link_libraries(pmm_simulator simgrid)
  target_link_libraries(pmm_slave gras)
  target_link_libraries(pmm_master gras)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_rl.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_sg_32.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_sg_64.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/pmm.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/pmm.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/make_deployment.pl
  PARENT_SCOPE
  )
