cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/_all2all_simulator.c
							${CMAKE_CURRENT_BINARY_DIR}/_all2all_sender.c
							${CMAKE_CURRENT_BINARY_DIR}/_all2all_receiver.c
							PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(all2all_simulator 	${CMAKE_CURRENT_BINARY_DIR}/_all2all_simulator.c
									${CMAKE_CURRENT_SOURCE_DIR}/all2all.c)
add_executable(all2all_sender 	${CMAKE_CURRENT_BINARY_DIR}/_all2all_sender.c
								${CMAKE_CURRENT_SOURCE_DIR}/all2all.c)
add_executable(all2all_receiver ${CMAKE_CURRENT_BINARY_DIR}/_all2all_receiver.c
								${CMAKE_CURRENT_SOURCE_DIR}/all2all.c)
								
add_custom_command(
  					OUTPUT 	${CMAKE_CURRENT_BINARY_DIR}/_all2all_simulator.c
							${CMAKE_CURRENT_BINARY_DIR}/_all2all_sender.c
							${CMAKE_CURRENT_BINARY_DIR}/_all2all_receiver.c
  					DEPENDS gras_stub_generator ${CMAKE_CURRENT_SOURCE_DIR}/all2all.xml
  					COMMAND ${CMAKE_BINARY_DIR}/bin/gras_stub_generator all2all ${CMAKE_CURRENT_SOURCE_DIR}/all2all.xml
					)

### Add definitions for compile
target_link_libraries(all2all_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(all2all_sender gras pthread m -fprofile-arcs)
target_link_libraries(all2all_receiver gras pthread m -fprofile-arcs)
