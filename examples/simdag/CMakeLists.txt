cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(ex_sd_test sd_test.c)
add_executable(sd_test2 sd_test2.c)
add_executable(sd_seq_access sd_seq_access.c)
add_executable(sd_test_console sd_test_console.c)
add_executable(simdag_tracing simdag_trace.c)

### Add definitions for compile
if(NOT WIN32)
	target_link_libraries(ex_sd_test simgrid pthread m )
	target_link_libraries(sd_test2 simgrid pthread m )
	target_link_libraries(sd_seq_access simgrid pthread m )
	target_link_libraries(sd_test_console simgrid pthread m )
	target_link_libraries(simdag_tracing simgrid pthread m )
	
	add_custom_command(TARGET ex_sd_test
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/ex_sd_test ${CMAKE_CURRENT_BINARY_DIR}/sd_test
		WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
	)
	get_directory_property(extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
	set_directory_properties(
	 PROPERTIES
	 ADDITIONAL_MAKE_CLEAN_FILES "${extra_clean_files};${CMAKE_CURRENT_BINARY_DIR}/sd_test")
 
else(NOT WIN32)
	target_link_libraries(ex_sd_test simgrid)
	target_link_libraries(sd_test2 simgrid)
	target_link_libraries(sd_seq_access simgrid)
	target_link_libraries(sd_test_console simgrid)
	target_link_libraries(simdag_tracing simgrid)
	
	add_custom_command(TARGET ex_sd_test
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/ex_sd_test.exe ${CMAKE_CURRENT_BINARY_DIR}/sd_test.exe
		WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
	)
	get_directory_property(extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
	set_directory_properties(
	 PROPERTIES
	 ADDITIONAL_MAKE_CLEAN_FILES "${extra_clean_files};${CMAKE_CURRENT_BINARY_DIR}/sd_test.exe")
endif(NOT WIN32)

