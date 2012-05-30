cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(masterslave_virtual_machines "masterslave_virtual_machines.c")

### Add definitions for compile
if(WIN32)
  target_link_libraries(masterslave_virtual_machines simgrid )
else(WIN32)
  target_link_libraries(masterslave_virtual_machines simgrid m )
endif(WIN32)
