cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(masterslave_failure "masterslave_failure.c")
add_executable(masterslave_forwarder "masterslave_forwarder.c")
add_executable(masterslave_mailbox "masterslave_mailbox.c")
add_executable(masterslave_bypass "masterslave_bypass.c")
add_executable(masterslave_console "masterslave_console.c")
add_executable(masterslave_cluster "masterslave_cluster.c")
add_executable(masterslave_kill "masterslave_kill.c")
add_executable(masterslave_arg "masterslave_arg.c")

### Add definitions for compile
if(WIN32)
  target_link_libraries(masterslave_forwarder simgrid )
  target_link_libraries(masterslave_failure simgrid )
  target_link_libraries(masterslave_mailbox simgrid )
  target_link_libraries(masterslave_bypass simgrid )
  target_link_libraries(masterslave_console simgrid )
  target_link_libraries(masterslave_kill simgrid )
  target_link_libraries(masterslave_arg simgrid )
else(WIN32)
  target_link_libraries(masterslave_forwarder simgrid m )
  target_link_libraries(masterslave_failure simgrid m )
  target_link_libraries(masterslave_mailbox simgrid m )
  target_link_libraries(masterslave_bypass simgrid m )
  target_link_libraries(masterslave_console simgrid m )
  target_link_libraries(masterslave_kill simgrid m )
  target_link_libraries(masterslave_arg simgrid m )
endif(WIN32)
target_link_libraries(masterslave_cluster simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_bypass.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_cpu_ti.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_forwarder_crosstraffic.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_mailbox.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_cas01_im.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_crosstraffic.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_forwarder.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_multicore.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_console.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_failure_crosstraffic.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_kill.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_cpu_ti_crosstraffic.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_failure.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_mailbox_crosstraffic.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_vivaldi.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave_forwarder.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave_mailbox_multicore.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave_vivaldi.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_clusters.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave_kill.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave_mailbox.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/deployment_masterslave.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_kill.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_arg.c
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_bypass.c
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_cluster.c
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_console.c
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_failure.c
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_forwarder.c
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_kill.c
  ${CMAKE_CURRENT_SOURCE_DIR}/masterslave_mailbox.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_script.lua
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  PARENT_SCOPE
  )
