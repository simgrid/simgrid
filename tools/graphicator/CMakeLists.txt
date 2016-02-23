add_executable       (graphicator graphicator.c)
target_link_libraries(graphicator simgrid)

## Clean generated files
get_directory_property(extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
set_directory_properties(
  PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
  "${extra_clean_files};${CMAKE_CURRENT_BINARY_DIR}/simgrid.trace;")

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/graphicator.tesh
  PARENT_SCOPE
  )
set(tools_src
  ${tools_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/graphicator.c
  PARENT_SCOPE
  )
