cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${PROJECT_DIRECTORY}/examples/gras/spawn/_spawn_simulator.c
							${PROJECT_DIRECTORY}/examples/gras/spawn/_spawn_child.c
							${PROJECT_DIRECTORY}/examples/gras/spawn/_spawn_father.c
							PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(spawn_simulator 	${CMAKE_CURRENT_BINARY_DIR}/_spawn_simulator.c 
								${CMAKE_CURRENT_SOURCE_DIR}/spawn_father.c 
								${CMAKE_CURRENT_SOURCE_DIR}/spawn_child.c 
								${CMAKE_CURRENT_SOURCE_DIR}/spawn_common.c)
add_executable(spawn_child 		${CMAKE_CURRENT_BINARY_DIR}/_spawn_child.c 
								${CMAKE_CURRENT_SOURCE_DIR}/spawn_child.c 
								${CMAKE_CURRENT_SOURCE_DIR}/spawn_common.c)
add_executable(spawn_father 	${CMAKE_CURRENT_BINARY_DIR}/_spawn_father.c
								${CMAKE_CURRENT_SOURCE_DIR}/spawn_father.c
								${CMAKE_CURRENT_SOURCE_DIR}/spawn_child.c
								${CMAKE_CURRENT_SOURCE_DIR}/spawn_common.c)

add_custom_command(
  					OUTPUT 	${CMAKE_CURRENT_BINARY_DIR}/_spawn_child.c 
         					${CMAKE_CURRENT_BINARY_DIR}/_spawn_father.c 
	 						${CMAKE_CURRENT_BINARY_DIR}/_spawn_simulator.c
  					DEPENDS gras_stub_generator ${CMAKE_CURRENT_SOURCE_DIR}/spawn.xml
  					COMMAND ${CMAKE_BINARY_DIR}/bin/gras_stub_generator --extra-process=child spawn ${CMAKE_CURRENT_SOURCE_DIR}/spawn.xml
					)
					
### Add definitions for compile
target_link_libraries(spawn_simulator simgrid pthread m )
target_link_libraries(spawn_child gras pthread m )
target_link_libraries(spawn_father gras pthread m )



