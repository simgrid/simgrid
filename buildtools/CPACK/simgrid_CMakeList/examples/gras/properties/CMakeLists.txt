cmake_minimum_required(VERSION 2.6)

set_source_files_properties(_properties_simulator.c _properties_alice.c _properties_bob.c
PROPERTIES GENERATED true)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(properties_simulator _properties_simulator.c properties.c)
add_executable(properties_alice _properties_alice.c    properties.c)
add_executable(properties_bob _properties_bob.c    properties.c)

### Add definitions for compile
target_link_libraries(properties_simulator simgrid pthread m -fprofile-arcs)
target_link_libraries(properties_alice gras pthread m -fprofile-arcs)
target_link_libraries(properties_bob gras pthread m -fprofile-arcs)
