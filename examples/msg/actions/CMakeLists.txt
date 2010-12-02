cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(actions actions.c)

### Add definitions for compile
target_link_libraries(actions simgrid m )

### Get our action files in the archive
set(EXTRA_DIST ${EXTRA_DIST} 
	examples/msg/actions/actions_allReduce.txt
	examples/msg/actions/actions_barrier.txt
	examples/msg/actions/actions_bcast.txt
	examples/msg/actions/actions_reduce.txt
	examples/msg/actions/actions_split_p0.txt
	examples/msg/actions/actions_split_p1.txt
	examples/msg/actions/actions.txt
	examples/msg/actions/actions_with_isend.txt
	)