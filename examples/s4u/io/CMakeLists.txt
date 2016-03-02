add_executable       (s4u_io_test s4u_io_test.cpp)
target_link_libraries(s4u_io_test simgrid)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/s4u_io.tesh
  PARENT_SCOPE)
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/s4u_io_test.cpp
  PARENT_SCOPE)
