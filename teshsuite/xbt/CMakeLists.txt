cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
if(NOT WIN32)
  add_executable(log_large_test log_large_test.c)
  add_executable(parallel_log_crashtest parallel_log_crashtest.c)
  if(HAVE_MMAP)
    add_executable(mmalloc_test mmalloc_test.c)
  endif()
endif()

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(log_large_test gras m pthread )
  target_link_libraries(parallel_log_crashtest gras m pthread )
  if(HAVE_MMAP)
    target_link_libraries(mmalloc_test gras m pthread )
  endif()
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/log_large_test.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/parallel_log_crashtest.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/xbt.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/mmalloc_64.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/mmalloc_32.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  PARENT_SCOPE
  )
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/log_large_test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/parallel_log_crashtest.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mmalloc_test.c
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
