cmake_minimum_required(VERSION 2.6)

set_source_files_properties(
  ${CMAKE_CURRENT_BINARY_DIR}/_properties_simulator.c
  ${CMAKE_CURRENT_BINARY_DIR}/_properties_alice.c
  ${CMAKE_CURRENT_BINARY_DIR}/_properties_bob.c
  PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(properties_simulator ${CMAKE_CURRENT_BINARY_DIR}/_properties_simulator.c ${CMAKE_CURRENT_SOURCE_DIR}/properties.c)
add_executable(properties_alice     ${CMAKE_CURRENT_BINARY_DIR}/_properties_alice.c ${CMAKE_CURRENT_SOURCE_DIR}/properties.c)
add_executable(properties_bob       ${CMAKE_CURRENT_BINARY_DIR}/_properties_bob.c ${CMAKE_CURRENT_SOURCE_DIR}/properties.c)
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/_properties_simulator.c
    ${CMAKE_CURRENT_BINARY_DIR}/_properties_alice.c
    ${CMAKE_CURRENT_BINARY_DIR}/_properties_bob.c
  DEPENDS gras_stub_generator ${CMAKE_CURRENT_SOURCE_DIR}/properties.xml
  COMMAND ${CMAKE_BINARY_DIR}/bin/gras_stub_generator properties ${CMAKE_CURRENT_SOURCE_DIR}/properties.xml
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(properties_simulator simgrid pthread m )
  target_link_libraries(properties_alice gras pthread m )
  target_link_libraries(properties_bob gras pthread m )
else(NOT WIN32)
  target_link_libraries(properties_simulator simgrid)
  target_link_libraries(properties_alice gras)
  target_link_libraries(properties_bob gras)
endif(NOT WIN32)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_rl.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_sg.tesh
  PARENT_SCOPE
)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/properties.xml
  PARENT_SCOPE
)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/properties.c
  PARENT_SCOPE
)
set(bin_files
  ${bin_files}
  PARENT_SCOPE
)


