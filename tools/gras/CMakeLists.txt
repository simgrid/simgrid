cmake_minimum_required(VERSION 2.6)

add_executable(gras_stub_generator stub_generator.c unix_stub_generator.c windows_stub_generator.c)

### Add definitions for compile
target_link_libraries(gras_stub_generator simgrid pthread m -fprofile-arcs)