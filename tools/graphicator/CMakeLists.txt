cmake_minimum_required(VERSION 2.6)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

add_executable(graphicator graphicator.c)

### Add definitions for compile
if(NOT WIN32)
  target_link_libraries(graphicator simgrid m pthread )
else()
  target_link_libraries(graphicator simgrid )
endif()

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
set(xml_files
  ${xml_files}
  PARENT_SCOPE
  )
set(tools_src
  ${tools_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/graphicator.c
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
