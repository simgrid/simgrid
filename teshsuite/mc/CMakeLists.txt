cmake_minimum_required(VERSION 2.6)

if(HAVE_MC)
  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
endif()

add_executable(mutex_handling mutex_handling.c)
target_link_libraries(mutex_handling simgrid)

add_executable(no_mutex_handling mutex_handling.c)
target_link_libraries(no_mutex_handling simgrid)
set_target_properties(no_mutex_handling PROPERTIES COMPILE_FLAGS -DDISABLE_THE_MUTEX=1)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/mutex_handling.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/no_mutex_handling.tesh
  PARENT_SCOPE
  )
set(testsuite_src
  ${testsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/mutex_handling.c
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/mutex_handling.xml
  PARENT_SCOPE
  )
