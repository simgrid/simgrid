cmake_minimum_required(VERSION 2.6)

set_source_files_properties(_chrono_simulator.c _chrono_multiplier.c
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(chrono_simulator _chrono_simulator.c chrono.c)
add_executable(chrono_multiplier _chrono_multiplier.c chrono.c)

### Add definitions for compile
target_link_libraries(chrono_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(chrono_multiplier gras pthread m -fprofile-arcs)
