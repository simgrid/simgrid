set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(masterslave_failure "masterslave_failure.c")
add_executable(masterslave_forwarder "masterslave_forwarder.c")
add_executable(masterslave_mailbox "masterslave_mailbox.c")
add_executable(masterslave_kill "masterslave_kill.c")
add_executable(masterslave_arg "masterslave_arg.c")

target_link_libraries(masterslave_forwarder simgrid )
target_link_libraries(masterslave_failure simgrid )
target_link_libraries(masterslave_mailbox simgrid )
target_link_libraries(masterslave_kill simgrid )
target_link_libraries(masterslave_arg simgrid )

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_cpu_ti.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_forwarder_crosstraffic.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_mailbox.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_cas01_im.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_crosstraffic.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_forwarder.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_multicore.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_failure_crosstraffic.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_kill.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_cpu_ti_crosstraffic.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_failure.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_mailbox_crosstraffic.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_vivaldi.tesh
  PARENT_SCOPE)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave_forwarder.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave_mailbox_multicore.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave_vivaldi.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave_kill.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave_mailbox.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave.xml
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_arg.c
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_failure.c
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_forwarder.c
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_kill.c
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_mailbox.c
  PARENT_SCOPE)

