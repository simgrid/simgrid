cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/examples/msg/masterslave/")

add_executable(masterslave_failure "masterslave_failure.c")
add_executable(masterslave_forwarder "masterslave_forwarder.c")
add_executable(masterslave_mailbox "masterslave_mailbox.c")
add_executable(masterslave_bypass "masterslave_bypass.c")

### Add definitions for compile
target_link_libraries(masterslave_forwarder simgrid m -fprofile-arcs)
target_link_libraries(masterslave_failure simgrid m -fprofile-arcs)
target_link_libraries(masterslave_mailbox simgrid m -fprofile-arcs)
target_link_libraries(masterslave_bypass simgrid m -fprofile-arcs)
