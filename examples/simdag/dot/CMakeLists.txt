cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/simdag/dot")			
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(dot_test dot_test.c)					#add_executable(<name_of_target> <src list>)

### Add definitions for compile
target_link_libraries(dot_test simgrid m pthread -fprofile-arcs) 	#target_link_libraries(<name_of_targe> <dependencies>)
