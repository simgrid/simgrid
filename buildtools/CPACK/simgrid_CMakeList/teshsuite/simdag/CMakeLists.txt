cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "./")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(basic0 basic0.c)
add_executable(basic1 basic1.c)
add_executable(basic2 basic2.c)
add_executable(basic3 basic3.c)
add_executable(basic4 basic4.c)
add_executable(basic5 basic5.c)
add_executable(basic6 basic6.c)

### Add definitions for compile
target_link_libraries(basic0 simgrid m pthread -fprofile-arcs)
target_link_libraries(basic1 simgrid m pthread -fprofile-arcs)
target_link_libraries(basic2 simgrid m pthread -fprofile-arcs)
target_link_libraries(basic3 simgrid m pthread -fprofile-arcs)
target_link_libraries(basic4 simgrid m pthread -fprofile-arcs)
target_link_libraries(basic5 simgrid m pthread -fprofile-arcs)
target_link_libraries(basic6 simgrid m pthread -fprofile-arcs)
