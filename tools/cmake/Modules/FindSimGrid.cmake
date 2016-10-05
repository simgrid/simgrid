#IF YOU HAVE INSTALL SIMGRID IN A SPECIAL DIRECTORY
#YOU CAN SPECIFY SIMGRID_ROOT

# TO CALL THIS FILE USE
#set(CMAKE_MODULE_PATH
#${CMAKE_MODULE_PATH}
#${CMAKE_HOME_DIRECTORY}/tools/cmake/Modules
#)

find_library(HAVE_SIMGRID_LIB
  NAME simgrid
  HINTS
  $ENV{LD_LIBRARY_PATH}
  $ENV{SIMGRID_ROOT}
  PATH_SUFFIXES lib64 lib
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr
  )

find_program(HAVE_TESH
  NAMES tesh
  HINTS
  $ENV{SIMGRID_ROOT}
  PATH_SUFFIXES bin
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr
  )

message(STATUS "Looking for lib SimGrid")
if(HAVE_SIMGRID_LIB)
  message(STATUS "Looking for lib SimGrid - found")
  get_filename_component(simgrid_version ${HAVE_SIMGRID_LIB} REALPATH)
  string(REPLACE "${HAVE_SIMGRID_LIB}." "" simgrid_version "${simgrid_version}")
  string(REGEX MATCH "^[0-9]" SIMGRID_MAJOR_VERSION "${simgrid_version}")
  string(REGEX MATCH "^[0-9].[0-9]" SIMGRID_MINOR_VERSION "${simgrid_version}")
  string(REGEX MATCH "^[0-9].[0-9].[0-9]" SIMGRID_PATCH_VERSION "${simgrid_version}")
  string(REGEX REPLACE "^${SIMGRID_MINOR_VERSION}." "" SIMGRID_PATCH_VERSION "${SIMGRID_PATCH_VERSION}")
  string(REGEX REPLACE "^${SIMGRID_MAJOR_VERSION}." "" SIMGRID_MINOR_VERSION "${SIMGRID_MINOR_VERSION}")
  message(STATUS "Simgrid version : ${SIMGRID_MAJOR_VERSION}.${SIMGRID_MINOR_VERSION}")
else()
  message(STATUS "Looking for lib SimGrid - not found")
endif()

if(HAVE_TESH)
  message(STATUS "Found Tesh: ${HAVE_TESH}")
endif()
