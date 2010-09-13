cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/_small_sleep_function.c
							${CMAKE_CURRENT_BINARY_DIR}/_small_sleep_simulator.c
							PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(small_sleep_function 	${CMAKE_CURRENT_SOURCE_DIR}/small_sleep.c
										${CMAKE_CURRENT_BINARY_DIR}/_small_sleep_function.c)
add_executable(small_sleep_simulator 	${CMAKE_CURRENT_SOURCE_DIR}/small_sleep.c
										${CMAKE_CURRENT_BINARY_DIR}/_small_sleep_simulator.c)

add_custom_command(
  					OUTPUT 	${CMAKE_CURRENT_BINARY_DIR}/_small_sleep_function.c
							${CMAKE_CURRENT_BINARY_DIR}/_small_sleep_simulator.c
  					DEPENDS gras_stub_generator ${CMAKE_CURRENT_SOURCE_DIR}/small_sleep.xml
  					COMMAND ${CMAKE_BINARY_DIR}/bin/gras_stub_generator small_sleep ${CMAKE_CURRENT_SOURCE_DIR}/small_sleep.xml
					)
					
### Add definitions for compile
if(NOT WIN32)
target_link_libraries(small_sleep_simulator simgrid m pthread)
target_link_libraries(small_sleep_function gras m pthread)
else(NOT WIN32)
target_link_libraries(small_sleep_simulator simgrid)
target_link_libraries(small_sleep_function gras)
endif(NOT WIN32)