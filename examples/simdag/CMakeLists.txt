cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/simdag/")

add_executable(ex_sd_test sd_test.c)
add_executable(sd_test2 sd_test2.c)
add_executable(sd_seq_access sd_seq_access.c)
add_executable(sd_test_console sd_test_console.c)

### Add definitions for compile
if(NOT WIN32)
	target_link_libraries(ex_sd_test simgrid pthread m )
	target_link_libraries(sd_test2 simgrid pthread m )
	target_link_libraries(sd_seq_access simgrid pthread m )
	target_link_libraries(sd_test_console simgrid pthread m )
	
	add_custom_command(TARGET ex_sd_test
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_DIRECTORY}/examples/simdag/ex_sd_test ${PROJECT_DIRECTORY}/examples/simdag/sd_test
		WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/simdag/"
	)
	get_directory_property(extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
	set_directory_properties(
	 PROPERTIES
	 ADDITIONAL_MAKE_CLEAN_FILES "${extra_clean_files};${PROJECT_DIRECTORY}/examples/simdag/sd_test")
 
else(NOT WIN32)
	target_link_libraries(ex_sd_test simgrid)
	target_link_libraries(sd_test2 simgrid)
	target_link_libraries(sd_seq_access simgrid)
	target_link_libraries(sd_test_console simgrid)
	
	add_custom_command(TARGET ex_sd_test
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_DIRECTORY}/examples/simdag/ex_sd_test.exe ${PROJECT_DIRECTORY}/examples/simdag/sd_test.exe
		WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/simdag/"
	)
	get_directory_property(extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
	set_directory_properties(
	 PROPERTIES
	 ADDITIONAL_MAKE_CLEAN_FILES "${extra_clean_files};${PROJECT_DIRECTORY}/examples/simdag/sd_test.exe")
endif(NOT WIN32)

