cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/_empty_main_function.c
  ${CMAKE_CURRENT_BINARY_DIR}/_empty_main_simulator.c
  PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(empty_main_function 	${CMAKE_CURRENT_SOURCE_DIR}/empty_main.c
  ${CMAKE_CURRENT_BINARY_DIR}/_empty_main_function.c)
add_executable(empty_main_simulator 	${CMAKE_CURRENT_SOURCE_DIR}/empty_main.c
  ${CMAKE_CURRENT_BINARY_DIR}/_empty_main_simulator.c)

add_custom_command(
  OUTPUT 	${CMAKE_CURRENT_BINARY_DIR}/_empty_main_function.c
  ${CMAKE_CURRENT_BINARY_DIR}/_empty_main_simulator.c
  DEPENDS gras_stub_generator ${CMAKE_CURRENT_SOURCE_DIR}/empty_main.xml
  COMMAND ${CMAKE_BINARY_DIR}/bin/gras_stub_generator empty_main ${CMAKE_CURRENT_SOURCE_DIR}/empty_main.xml
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(empty_main_simulator simgrid m pthread)
  target_link_libraries(empty_main_function gras m pthread)
else()
  target_link_libraries(empty_main_simulator simgrid)
  target_link_libraries(empty_main_function gras)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_rl.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_sg.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/empty_main.xml
  PARENT_SCOPE
  )
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/empty_main.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  PARENT_SCOPE
  )
