cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(get_sender ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/get_sender.c)
add_executable(pid ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/pid.c)
add_executable(host_on_off ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/host_on_off.c)
add_executable(task_destroy_cancel ${CMAKE_HOME_DIRECTORY}/teshsuite/msg/task_destroy_cancel.c)


### Add definitions for compile
target_link_libraries(get_sender simgrid)
target_link_libraries(host_on_off simgrid)
target_link_libraries(task_destroy_cancel simgrid)
if(NOT WIN32)
  target_link_libraries(pid simgrid m pthread)
else()
  target_link_libraries(pid simgrid)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/get_sender.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/pid.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/task_destroy_cancel.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/get_sender_d.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/get_sender_p.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/pid_d.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/pid_p.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off_d.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off_p.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/task_destroy_cancel_d.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/task_destroy_cancel_p.xml
  PARENT_SCOPE
  )
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/get_sender.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pid.c
  ${CMAKE_CURRENT_SOURCE_DIR}/host_on_off.c
  ${CMAKE_CURRENT_SOURCE_DIR}/task_destroy_cancel.c
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  PARENT_SCOPE
  )
