cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/src/")
set(USE_TEST_UNITS "${PROJECT_DIRECTORY}/src/simgrid_units_main.c")

foreach(file ${TEST_UNITS})
	set(USE_TEST_UNITS
	${USE_TEST_UNITS}
	${PROJECT_DIRECTORY}/src/${file}
	)
endforeach(file ${TEST_UNITS})

set_source_files_properties(${USE_TEST_UNITS}
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "./")

add_executable(testall ${USE_TEST_UNITS})

### Add definitions for compile
target_link_libraries(testall gras m -fprofile-arcs)
