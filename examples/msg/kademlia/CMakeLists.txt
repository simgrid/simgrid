cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(kademlia "kademlia.c" "node.c" "routing_table.c" 
"task.c" "answer.c")
### Add definitions for compile
target_link_libraries(kademlia simgrid )
