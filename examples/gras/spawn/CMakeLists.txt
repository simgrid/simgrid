cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/_spawn_simulator.c
			                ${CMAKE_CURRENT_BINARY_DIR}/_spawn_server.c
			                PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(spawn_simulator 	${CMAKE_CURRENT_BINARY_DIR}/_spawn_simulator.c 
				${CMAKE_CURRENT_SOURCE_DIR}/spawn.c)
add_executable(spawn_server	${CMAKE_CURRENT_BINARY_DIR}/_spawn_server.c 
				${CMAKE_CURRENT_SOURCE_DIR}/spawn.c)

add_custom_command(OUTPUT   ${CMAKE_CURRENT_BINARY_DIR}/_spawn_server.c
                            ${CMAKE_CURRENT_BINARY_DIR}/_spawn_simulator.c
                   DEPENDS gras_stub_generator ${CMAKE_CURRENT_SOURCE_DIR}/spawn.xml
                   COMMAND ${CMAKE_BINARY_DIR}/bin/gras_stub_generator spawn ${CMAKE_CURRENT_SOURCE_DIR}/spawn.xml
                  )
          
### Add definitions for compile
if(NOT WIN32)
target_link_libraries(spawn_simulator simgrid pthread m )
target_link_libraries(spawn_server gras pthread m )
else(NOT WIN32)
target_link_libraries(spawn_simulator simgrid)
target_link_libraries(spawn_server gras)
endif(NOT WIN32)
