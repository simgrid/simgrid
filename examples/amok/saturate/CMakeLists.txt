cmake_minimum_required(VERSION 2.6)

set_source_files_properties(
  ${CMAKE_CURRENT_BINARY_DIR}/_saturate_simulator.c
  ${CMAKE_CURRENT_BINARY_DIR}/_saturate_maestro.c
  ${CMAKE_CURRENT_BINARY_DIR}/_saturate_sensor.c
  PROPERTIES GENERATED true
  )

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
add_executable(saturate_simulator ${CMAKE_CURRENT_BINARY_DIR}/_saturate_simulator.c ${CMAKE_CURRENT_SOURCE_DIR}/saturate.c)
add_executable(saturate_maestro   ${CMAKE_CURRENT_BINARY_DIR}/_saturate_maestro.c ${CMAKE_CURRENT_SOURCE_DIR}/saturate.c)
add_executable(saturate_sensor    ${CMAKE_CURRENT_BINARY_DIR}/_saturate_sensor.c ${CMAKE_CURRENT_SOURCE_DIR}/saturate.c)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_saturate_simulator.c
  ${CMAKE_CURRENT_BINARY_DIR}/_saturate_maestro.c
  ${CMAKE_CURRENT_BINARY_DIR}/_saturate_sensor.c
  DEPENDS gras_stub_generator ${CMAKE_CURRENT_SOURCE_DIR}/saturate.xml
  COMMAND ${CMAKE_BINARY_DIR}/bin/gras_stub_generator saturate ${CMAKE_CURRENT_SOURCE_DIR}/saturate.xml
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(saturate_simulator simgrid pthread m)
  target_link_libraries(saturate_maestro gras pthread m)
  target_link_libraries(saturate_sensor gras pthread m)
else()
  target_link_libraries(saturate_simulator simgrid)
  target_link_libraries(saturate_maestro gras)
  target_link_libraries(saturate_sensor gras)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/saturate_rl.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/saturate_sg_32.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/saturate_sg_64.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/saturate.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/env.c
  ${CMAKE_CURRENT_SOURCE_DIR}/saturate.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
