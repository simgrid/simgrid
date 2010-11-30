cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

add_executable(tesh tesh.c run_context.c signal.c)

### Add definitions for compile
target_link_libraries(tesh gras m pthread) 
