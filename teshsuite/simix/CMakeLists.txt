cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(check_defaults check_defaults.c)
target_link_libraries(check_defaults simgrid)

add_executable(stack_overflow stack_overflow.c)
target_link_libraries(stack_overflow simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/factory_raw.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/factory_thread.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/factory_ucontext.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  PARENT_SCOPE
  )
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/check_defaults.c
  ${CMAKE_CURRENT_SOURCE_DIR}/stack_overflow.c
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
