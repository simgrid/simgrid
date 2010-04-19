cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/tools/tesh/")

add_executable(tesh ${PROJECT_DIRECTORY}/tools/tesh/tesh.c ${PROJECT_DIRECTORY}/tools/tesh/run_context.c ${PROJECT_DIRECTORY}/tools/tesh/signal.c)

### Add definitions for compile
target_link_libraries(tesh gras m pthread -fprofile-arcs) 
