cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/msg/mc/")

add_executable(centralized centralized_mutex.c)
add_executable(bugged1     bugged1.c)
add_executable(bugged2     bugged2.c)
add_executable(bugged3	   bugged3.c)
add_executable(random_test random_test.c)

target_link_libraries(centralized simgrid m )
target_link_libraries(bugged1     simgrid m )
target_link_libraries(bugged2     simgrid m )
target_link_libraries(bugged3     simgrid m )
target_link_libraries(random_test     simgrid m )
