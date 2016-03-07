add_executable       (graphicator graphicator.c)
target_link_libraries(graphicator simgrid)
set_target_properties(graphicator PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
ADD_TESH(graphicator --setenv srcdir=${CMAKE_HOME_DIRECTORY} --setenv bindir=${CMAKE_BINARY_DIR}/bin --cd ${CMAKE_HOME_DIRECTORY}/tools/graphicator graphicator.tesh)

## Clean generated files
get_directory_property(extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${extra_clean_files};${CMAKE_CURRENT_BINARY_DIR}/simgrid.trace;")

set(tesh_files  ${tesh_files}  ${CMAKE_CURRENT_SOURCE_DIR}/graphicator.tesh  PARENT_SCOPE)
set(tools_src   ${tools_src}   ${CMAKE_CURRENT_SOURCE_DIR}/graphicator.c     PARENT_SCOPE)
