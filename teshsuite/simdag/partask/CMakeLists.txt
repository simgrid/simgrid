set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(test_comp_only_seq test_comp_only_seq.c)
add_executable(test_comp_only_par test_comp_only_par.c)
target_link_libraries(test_comp_only_seq simgrid)
target_link_libraries(test_comp_only_par simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_comp_only_par.tesh
  ${CMAKE_CURRENT_SOURCE_DIR}/test_comp_only_seq.tesh
  PARENT_SCOPE)
set(xml_files
  ${xml_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_2p_1sl_hetero.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/platform_2p_1sl.xml
  PARENT_SCOPE)
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_comp_only_par.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test_comp_only_seq.c
  PARENT_SCOPE)
