cmake_minimum_required(VERSION 2.6)

set_source_files_properties(${PROJECT_DIRECTORY}/src/simgrid_units_main.c
PROPERTIES GENERATED true)

foreach(file ${TEST_UNITS})
	set_source_files_properties(${file}
PROPERTIES GENERATED true)
	message("Generated file :  ${file}")
endforeach(file ${TEST_UNITS})


set(EXECUTABLE_OUTPUT_PATH "./")

add_executable(testall ${TEST_UNITS} ${PROJECT_DIRECTORY}/src/simgrid_units_main.c)

### Add definitions for compile
target_link_libraries(testall gras m -fprofile-arcs)
