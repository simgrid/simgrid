cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/_timer_simulator.c
							${CMAKE_CURRENT_BINARY_DIR}/_timer_client.c
							PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(timer_simulator 	${CMAKE_CURRENT_BINARY_DIR}/_timer_simulator.c 
								${CMAKE_CURRENT_SOURCE_DIR}/timer.c)
add_executable(timer_client 	${CMAKE_CURRENT_BINARY_DIR}/_timer_client.c
								${CMAKE_CURRENT_SOURCE_DIR}/timer.c)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_timer_simulator.c
         ${CMAKE_CURRENT_BINARY_DIR}/_timer_client.c
  DEPENDS gras_stub_generator ${CMAKE_CURRENT_SOURCE_DIR}/timer.xml
  COMMAND ${CMAKE_BINARY_DIR}/bin/gras_stub_generator timer ${CMAKE_CURRENT_SOURCE_DIR}/timer.xml
)

### Add definitions for compile
if(NOT WIN32)
target_link_libraries(timer_simulator simgrid pthread m )
target_link_libraries(timer_client gras pthread m )
else(NOT WIN32)
target_link_libraries(timer_simulator simgrid)
target_link_libraries(timer_client gras)
endif(NOT WIN32)

