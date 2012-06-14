cmake_minimum_required(VERSION 2.6)

set_source_files_properties(
  ${CMAKE_CURRENT_BINARY_DIR}/_simple_token_simulator.c
  ${CMAKE_CURRENT_BINARY_DIR}/_simple_token_node.c
  PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
add_executable(simple_token_simulator ${CMAKE_CURRENT_BINARY_DIR}/_simple_token_simulator.c ${CMAKE_CURRENT_SOURCE_DIR}/simple_token.c)
add_executable(simple_token_node      ${CMAKE_CURRENT_BINARY_DIR}/_simple_token_node.c ${CMAKE_CURRENT_SOURCE_DIR}/simple_token.c)

add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/_simple_token_simulator.c 
    ${CMAKE_CURRENT_BINARY_DIR}/_simple_token_node.c 
  DEPENDS gras_stub_generator ${CMAKE_CURRENT_SOURCE_DIR}/simple_token.xml
  COMMAND ${CMAKE_BINARY_DIR}/bin/gras_stub_generator simple_token ${CMAKE_CURRENT_SOURCE_DIR}/simple_token.xml
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(simple_token_simulator simgrid pthread m )
  target_link_libraries(simple_token_node gras pthread m )
else(NOT WIN32)
  target_link_libraries(simple_token_simulator simgrid)
  target_link_libraries(simple_token_node gras)
endif(NOT WIN32)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_rl.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_sg_32.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_sg_64.tesh
  PARENT_SCOPE
)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/simple_token.xml
  PARENT_SCOPE
)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/simple_token.c
  PARENT_SCOPE
)
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/run.sh
  ${CMAKE_CURRENT_SOURCE_DIR}/make_deployment.pl
  PARENT_SCOPE
)
