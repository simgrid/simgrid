#### Generate the html documentation
find_program(WGET_PROGRAM  NAMES wget)
find_program(NSIS_PROGRAM NAMES makensi)

message(STATUS "wget: ${WGET_PROGRAM}")
message(STATUS "nsis: ${NSIS_PROGRAM}")

if(WGET_PROGRAM)
  ADD_CUSTOM_TARGET(simgrid_documentation
    COMMENT "Downloading the SimGrid documentation..."
    COMMAND ${WGET_PROGRAM} -r -np -nH -nd http://simgrid.gforge.inria.fr/simgrid/${release_version}/doc/
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/html
    )
endif()

if(NSIS_PROGRAM)
  ADD_CUSTOM_TARGET(nsis
    COMMENT "Generating the SimGrid installer for Windows..."
    DEPENDS simgrid simgrid graphicator simgrid-colorizer simgrid_update_xml
    COMMAND ${NSIS_PROGRAM} simgrid.nsi
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/
    )
endif()
