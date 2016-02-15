set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(incomplete incomplete.c)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(incomplete simgrid m pthread )
else()
  target_link_libraries(incomplete simgrid)
endif()

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/incomplete.tesh
  PARENT_SCOPE)
set(teshsuite_src
  ${teshsuite_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/incomplete.c
  PARENT_SCOPE)
