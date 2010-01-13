cmake_minimum_required(VERSION 2.6)

#--------------------------------------------------------------------------------------------------
### need to set TODO make it by arguments
SET(disable_java 0)
SET(disable_gtnets 0)
SET(gtnets_path "")
SET(disable_context_threads 1)

### Set some variables for Cmake
SET(PROJECT_NAME "Simgrid_cmake")
SET(LANGUAGE "Cxx")
SET(PROJECT_DIRECTORY "/home/navarrop/Bureau/simgrid-trunk")
SET(PROJECT_SOURCE_DIRECTORY "${PROJECT_DIRECTORY}/src")
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/bin")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")
set(INCLUDES "-I${PROJECT_DIRECTORY}/include -I${PROJECT_DIRECTORY}/src/include -I${PROJECT_DIRECTORY}/src")

### Make the *.h files with *.h.in files
include(${PROJECT_DIRECTORY}/src/CMakeCompleteInFiles.txt OPTIONAL NO_POLICY_SCOPE)

### Define source packages for Libs 
include(${PROJECT_DIRECTORY}/src/CMakeDefinePackages.txt OPTIONAL NO_POLICY_SCOPE)

### Make Libs
add_definitions("${INCLUDES}")

add_library(simgrid SHARED ${simgrid_sources})
add_library(gras SHARED ${gras_sources})

### Make the distrib
install(TARGETS simgrid	DESTINATION "simgrid/lib/")
install(TARGETS gras	DESTINATION "simgrid/lib/")
install(FILES ${simgrid_sources} DESTINATION "simgrid/src/simgrid")
install(FILES ${gras_sources} DESTINATION "simgrid/src/gras")
install(FILES ${EXTRA_DIST} DESTINATION "simgrid/src/ExtraDist")
install(DIRECTORY "${PROJECT_DIRECTORY}/include/" 
	DESTINATION "simgrid/include/"
	)

set(CPACK_PROJECT_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}")
set(CPACK_GENERATOR "TGZ;TZ;ZIP")
include(CPack)


