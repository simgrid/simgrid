set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
add_executable(s4u_io_test ${CMAKE_CURRENT_SOURCE_DIR}/s4u_io_test.cpp)
target_link_libraries(s4u_io_test simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/s4u_io.tesh
  PARENT_SCOPE)
set(xml_files
  ${xml_files}
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/s4u_io_test.cpp
  PARENT_SCOPE)
set(bin_files
  ${bin_files}
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  PARENT_SCOPE)
