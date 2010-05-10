cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/simdag/")

add_executable(ex_sd_test sd_test.c)
add_executable(ex_sd_test2 sd_test2.c)
add_executable(ex_sd_seq_access sd_seq_access.c)

### Add definitions for compile
target_link_libraries(ex_sd_test simgrid pthread m -fprofile-arcs)
target_link_libraries(ex_sd_test2 simgrid pthread m -fprofile-arcs)
target_link_libraries(ex_sd_seq_access simgrid pthread m -fprofile-arcs)

add_custom_command(TARGET ex_sd_test
POST_BUILD
COMMAND "cp" 
ARGS ${PROJECT_DIRECTORY}/examples/simdag/ex_sd_test ${PROJECT_DIRECTORY}/examples/simdag/sd_test
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/simdag/"
)
add_custom_command(TARGET ex_sd_test2
POST_BUILD
COMMAND "cp" 
ARGS ${PROJECT_DIRECTORY}/examples/simdag/ex_sd_test2 ${PROJECT_DIRECTORY}/examples/simdag/sd_test2
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/simdag/"
)

add_custom_command(TARGET ex_sd_seq_access
POST_BUILD
COMMAND "cp" 
ARGS ${PROJECT_DIRECTORY}/examples/simdag/ex_sd_seq_access ${PROJECT_DIRECTORY}/examples/simdag/sd_seq_access
WORKING_DIRECTORY "${PROJECT_DIRECTORY}/examples/simdag/"
)
