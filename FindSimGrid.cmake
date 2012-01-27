#IF YOU HAVE INSTALL SIMGRID IN A SPECIAL DIRECTORY
#YOU CAN SPECIFY SIMGRID_ROOT OR GRAS_ROOT

find_library(HAVE_SIMGRID_LIB
    NAME simgrid
    HINTS
	$ENV{SIMGRID_ROOT}
    PATH_SUFFIXES lib
)

find_path(SIMGRID_INCLUDES msg.h
    HINTS
	$ENV{SIMGRID_ROOT}
    PATH_SUFFIXES include
)

message(STATUS "Looking for lib SimGrid")
if(HAVE_SIMGRID_LIB)
  message(STATUS "Looking for lib SimGrid - found")
  if(CMAKE_CACHE_MAJOR_VERSION EQUAL "2" AND CMAKE_CACHE_MINOR_VERSION GREATER "7") #need cmake version 2.8
	  get_filename_component(simgrid_version ${HAVE_SIMGRID_LIB} REALPATH)
	  string(REPLACE "${HAVE_SIMGRID_LIB}." "" simgrid_version "${simgrid_version}")
	  string(REGEX MATCH "^[0-9]" SIMGRID_MAJOR_VERSION "${simgrid_version}")
	  string(REGEX MATCH "^[0-9].[0-9]" SIMGRID_MINOR_VERSION "${simgrid_version}")
	  string(REGEX MATCH "^[0-9].[0-9].[0-9]" SIMGRID_PATCH_VERSION "${simgrid_version}")
	  string(REGEX REPLACE "^${SIMGRID_MINOR_VERSION}." "" SIMGRID_PATCH_VERSION "${SIMGRID_PATCH_VERSION}") 
	  string(REGEX REPLACE "^${SIMGRID_MAJOR_VERSION}." "" SIMGRID_MINOR_VERSION "${SIMGRID_MINOR_VERSION}")
	  message(STATUS "Simgrid version : ${SIMGRID_MAJOR_VERSION}.${SIMGRID_MINOR_VERSION}")
  endif(CMAKE_CACHE_MAJOR_VERSION EQUAL "2" AND CMAKE_CACHE_MINOR_VERSION GREATER "7")
else(HAVE_SIMGRID_LIB)
  message(STATUS "Looking for lib SimGrid - not found")
endif(HAVE_SIMGRID_LIB)

message(STATUS "Looking for msg.h")
if(SIMGRID_INCLUDES)
  message(STATUS "Looking for msg.h - found")
else(SIMGRID_INCLUDES)
  message(STATUS "Looking for msg.h - not found")
endif(SIMGRID_INCLUDES)

if(HAVE_SIMGRID_LIB AND SIMGRID_INCLUDES)
else(HAVE_SIMGRID_LIB AND SIMGRID_INCLUDES)
    message(FATAL_ERROR "Unable to find both the library and the include files. Setting the environment variable SIMGRID_ROOT may help.")
endif(HAVE_SIMGRID_LIB AND SIMGRID_INCLUDES)