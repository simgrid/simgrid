cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")			

add_executable(dot_test dot_test.c)					#add_executable(<name_of_target> <src list>)
add_executable(simulate_dot simulate_dot.c)					#add_executable(<name_of_target> <src list>)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(dot_test simgrid pthread m) 	#target_link_libraries(<name_of_targe> <dependencies>)
  target_link_libraries(simulate_dot simgrid pthread m) 	#target_link_libraries(<name_of_targe> <dependencies>)

	get_directory_property(extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
	set_directory_properties(
	 PROPERTIES
	 ADDITIONAL_MAKE_CLEAN_FILES "${extra_clean_files};${CMAKE_CURRENT_BINARY_DIR}/dot_test; ${CMAKE_CURRENT_BINARY_DIR}/simulate_dot;")
  
else(NOT WIN32)
  target_link_libraries(dot_test simgrid) 	#target_link_libraries(<name_of_targe> <dependencies>)
  target_link_libraries(simulate_dot simgrid) 	#target_link_libraries(<name_of_targe> <dependencies>)
	set_directory_properties(
	 PROPERTIES
	 ADDITIONAL_MAKE_CLEAN_FILES "${extra_clean_files};${CMAKE_CURRENT_BINARY_DIR}/dot_test; ${CMAKE_CURRENT_BINARY_DIR}/simulate_dot;")
endif(NOT WIN32)
