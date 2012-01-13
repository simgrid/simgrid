#### Generate the html documentation
find_path(WGET_PATH	NAMES wget.exe	PATHS NO_DEFAULT_PATHS)
find_path(NSIS_PATH NAMES makensis.exe  PATHS NO_DEFAULT_PATHS)

message(STATUS "wget: ${WGET_PATH}")
message(STATUS "nsis: ${NSIS_PATH}")

if(WGET_PATH)
	ADD_CUSTOM_TARGET(simgrid_documentation
		COMMENT "Downloading the SimGrid documentation..."
		COMMAND ${WGET_PATH}/wget.exe -r -np -nH -nd http://simgrid.gforge.inria.fr/simgrid/${release_version}/doc/
		WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/html
	)
endif(WGET_PATH)

if(NSIS_PATH)
ADD_CUSTOM_TARGET(nsis
	COMMENT "Generating the SimGrid installor for Windows..."
	DEPENDS simgrid simgrid_shared gras graphicator gras_stub_generator tesh simgrid-colorizer simgrid_update_xml
	COMMAND ${NSIS_PATH}/makensis.exe simgrid.nsi
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/
)
endif(NSIS_PATH)