cmake_minimum_required(VERSION 2.6)

set_source_files_properties(_all2all_simulator.c _all2all_sender.c _all2all_receiver.c
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(all2all_simulator _all2all_simulator.c all2all.c)
add_executable(all2all_sender _all2all_sender.c all2all.c)
add_executable(all2all_receiver _all2all_receiver.c all2all.c)

### Add definitions for compile
target_link_libraries(all2all_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(all2all_sender gras pthread m -fprofile-arcs)
target_link_libraries(all2all_receiver gras pthread m -fprofile-arcs)
