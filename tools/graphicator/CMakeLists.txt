add_executable       (graphicator graphicator.cpp)
add_dependencies     (tests       graphicator)
target_link_libraries(graphicator simgrid)
set_target_properties(graphicator PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
ADD_TESH(graphicator --setenv srcdir=${CMAKE_HOME_DIRECTORY} --setenv bindir=${CMAKE_BINARY_DIR}/bin
                     --cd ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/graphicator.tesh)

install(TARGETS graphicator DESTINATION ${CMAKE_INSTALL_BINDIR}/)

set(tesh_files  ${tesh_files}  ${CMAKE_CURRENT_SOURCE_DIR}/graphicator.tesh  PARENT_SCOPE)
set(tools_src   ${tools_src}   ${CMAKE_CURRENT_SOURCE_DIR}/graphicator.cpp   PARENT_SCOPE)
