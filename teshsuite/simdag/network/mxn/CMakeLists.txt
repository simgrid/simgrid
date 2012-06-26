cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(test_intra_all2all test_intra_all2all.c)
add_executable(test_intra_independent_comm test_intra_independent_comm.c)
add_executable(test_intra_scatter test_intra_scatter.c)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(test_intra_all2all simgrid m pthread )
  target_link_libraries(test_intra_independent_comm simgrid m pthread )
  target_link_libraries(test_intra_scatter simgrid m pthread )
else(NOT WIN32)
  target_link_libraries(test_intra_all2all simgrid)
  target_link_libraries(test_intra_independent_comm simgrid)
  target_link_libraries(test_intra_scatter simgrid)
endif(NOT WIN32)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_intra_all2all.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_intra_independent_comm.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_intra_scatter.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_4p_1switch.xml
  PARENT_SCOPE
  )
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_intra_all2all.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test_intra_independent_comm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test_intra_scatter.c
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
