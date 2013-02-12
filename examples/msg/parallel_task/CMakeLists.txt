cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(parallel_task parallel_task.c)
add_executable(test_ptask test_ptask.c)

### Add definitions for compile
target_link_libraries(parallel_task simgrid )
target_link_libraries(test_ptask simgrid )

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/parallel_task.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_ptask_deployment.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/test_ptask_platform.xml
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/parallel_task.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test_ptask.c
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
