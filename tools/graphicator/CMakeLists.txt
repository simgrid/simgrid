if(enable_MSG)
  add_executable       (graphicator graphicator.c)
  add_dependencies     (tests       graphicator)
  target_link_libraries(graphicator simgrid)
  set_target_properties(graphicator PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  ADD_TESH(graphicator --setenv srcdir=${CMAKE_HOME_DIRECTORY} --setenv bindir=${CMAKE_BINARY_DIR}/bin --cd ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/graphicator.tesh)

  install(TARGETS graphicator DESTINATION bin/)

  ## Clean generated files
  get_directory_property(extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
  set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${extra_clean_files};${CMAKE_CURRENT_BINARY_DIR}/simgrid.trace;")
endif()

set(tesh_files  ${tesh_files}  ${CMAKE_CURRENT_SOURCE_DIR}/graphicator.tesh  PARENT_SCOPE)
set(tools_src   ${tools_src}   ${CMAKE_CURRENT_SOURCE_DIR}/graphicator.c     PARENT_SCOPE)
