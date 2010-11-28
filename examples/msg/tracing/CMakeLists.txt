cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")

add_executable(ms         ${CMAKE_CURRENT_SOURCE_DIR}/ms.c)
add_executable(categories ${CMAKE_CURRENT_SOURCE_DIR}/categories.c)
add_executable(volume     ${CMAKE_CURRENT_SOURCE_DIR}/volume.c)
add_executable(tasks      ${CMAKE_CURRENT_SOURCE_DIR}/tasks.c)
add_executable(procmig    ${CMAKE_CURRENT_SOURCE_DIR}/procmig.c)

### Add definitions for compile
target_link_libraries(ms simgrid m )
target_link_libraries(categories simgrid m )
target_link_libraries(volume simgrid m )
target_link_libraries(tasks simgrid m )
target_link_libraries(procmig simgrid m )
