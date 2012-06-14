cmake_minimum_required(VERSION 2.6)

set_source_files_properties(
  ${CMAKE_CURRENT_BINARY_DIR}/_chrono_simulator.c
  ${CMAKE_CURRENT_BINARY_DIR}/_chrono_multiplier.c
  PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
add_executable(chrono_simulator  ${CMAKE_CURRENT_BINARY_DIR}/_chrono_simulator.c ${CMAKE_CURRENT_SOURCE_DIR}/chrono.c)
add_executable(chrono_multiplier ${CMAKE_CURRENT_BINARY_DIR}/_chrono_multiplier.c ${CMAKE_CURRENT_SOURCE_DIR}/chrono.c)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_chrono_simulator.c
  ${CMAKE_CURRENT_BINARY_DIR}/_chrono_multiplier.c
  DEPENDS gras_stub_generator ${CMAKE_CURRENT_SOURCE_DIR}/chrono.xml
  COMMAND ${CMAKE_BINARY_DIR}/bin/gras_stub_generator chrono ${CMAKE_CURRENT_SOURCE_DIR}/chrono.xml
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(chrono_simulator simgrid pthread m )
  target_link_libraries(chrono_multiplier gras pthread m )
else(NOT WIN32)
  target_link_libraries(chrono_simulator simgrid)
  target_link_libraries(chrono_multiplier gras)
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
  ${CMAKE_CURRENT_SOURCE_DIR}/chrono.xml
  PARENT_SCOPE
)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/chrono.c
  ${CMAKE_CURRENT_SOURCE_DIR}/chrono2.c
  PARENT_SCOPE
)
set(bin_files
  ${bin_files}
  PARENT_SCOPE
)
