#IF YOU HAVE INSTALL SIMGRID IN A SPECIAL DIRECTORY
#YOU CAN SPECIFY SIMGRID_ROOT OR GRAS_ROOT

find_library(SIMGRID_LIB
    NAME simgrid
    HINTS
	$ENV{SIMGRID_ROOT}
    PATH_SUFFIXES lib
)

find_path(SIMGRID_INCLUDES msg/msg.h
    HINTS
	$ENV{SIMGRID_ROOT}
    PATH_SUFFIXES include
)

find_program(TESH_BIN_PATH
	NAMES tesh
	HINTS
	$ENV{SIMGRID_ROOT}
	PATH_SUFFIXES bin
)

message(STATUS "Looking for lib SimGrid")
if(SIMGRID_LIB)
  message(STATUS "Looking for lib SimGrid - found")  
  get_filename_component(simgrid_version ${SIMGRID_LIB} REALPATH)
  string(REPLACE "${SIMGRID_LIB}." "" simgrid_version "${simgrid_version}")	  
  string(REGEX MATCH "^[0-9]" SIMGRID_MAJOR_VERSION "${simgrid_version}")
  string(REGEX MATCH "^[0-9].[0-9]" SIMGRID_MINOR_VERSION "${simgrid_version}")
  string(REGEX MATCH "^[0-9].[0-9].[0-9]" SIMGRID_PATCH_VERSION "${simgrid_version}")
  string(REGEX REPLACE "^${SIMGRID_MINOR_VERSION}." "" SIMGRID_PATCH_VERSION "${SIMGRID_PATCH_VERSION}") 
  string(REGEX REPLACE "^${SIMGRID_MAJOR_VERSION}." "" SIMGRID_MINOR_VERSION "${SIMGRID_MINOR_VERSION}")
else(SIMGRID_LIB)
  message(STATUS "Looking for lib SimGrid - not found")
endif(SIMGRID_LIB)

string(REGEX REPLACE "libsimgrid.*" "" SIMGRID_LIB_PATH "${SIMGRID_LIB}")

message(STATUS "Simgrid         : ${SIMGRID_LIB}")
message(STATUS "Simgrid_path    : ${SIMGRID_LIB_PATH}")
message(STATUS "Simgrid version : ${SIMGRID_MAJOR_VERSION}.${SIMGRID_MINOR_VERSION}")

message(STATUS "Looking for msg.h")
if(SIMGRID_INCLUDES)
  message(STATUS "Looking for msg.h - found")
else(SIMGRID_INCLUDES)
  message(STATUS "Looking for msg.h - not found")
endif(SIMGRID_INCLUDES)

if(SIMGRID_LIB AND SIMGRID_INCLUDES)
else(SIMGRID_LIB AND SIMGRID_INCLUDES)
    message(FATAL_ERROR "Unable to find both the library and the include files. Setting the environment variable SIMGRID_ROOT may help.")
endif(SIMGRID_LIB AND SIMGRID_INCLUDES)

if(TESH_BIN_PATH)
message(STATUS "Found Tesh: ${TESH_BIN_PATH}")
endif(TESH_BIN_PATH)