# Try to find PAPI headers and LIBRARY.
#
# Usage of this module as follows:
#
#     find_package(PAPI)
#
# Variables used by this module, they can change the default behavior and need
# to be set before calling find_package:
#
#  PAPI_PREFIX         Set this variable to the root installation of
#                      libpapi if the module has problems finding the
#                      proper installation path.
#
# Variables defined by this module:
#
#  PAPI_FOUND              System has PAPI LIBRARY and headers
#  PAPI_LIBRARY          The PAPI library
#  PAPI_INCLUDE_DIRS       The location of PAPI headers

set(HAVE_PAPI 0)
set(PAPI_HINT ${papi_path} CACHE PATH "Path to search for PAPI headers and library")

find_path(PAPI_PREFIX
    NAMES include/papi.h
	PATHS
	${PAPI_HINT}
)

message(STATUS "Looking for libpapi")
find_library(PAPI_LIBRARY
    NAMES libpapi papi
    PATH_SUFFIXES lib64 lib
    # HINTS gets searched before PATHS
    HINTS
    ${PAPI_PREFIX}/lib
)
if(PAPI_LIBRARY)
  message(STATUS "Looking for libpapi - found at ${PAPI_LIBRARY}")
else()
  message(STATUS "Looking for libpapi - not found")
endif()

message(STATUS "Looking for papi.h")
find_path(PAPI_INCLUDE_DIRS
    NAMES papi.h
    # HINTS gets searched before PATHS
    HINTS ${PAPI_PREFIX}/include
)
if(PAPI_INCLUDE_DIRS)
  message(STATUS "Looking for papi.h - found at ${PAPI_INCLUDE_DIRS}")
else()
  message(STATUS "Looking for papi.h - not found")
endif()


if (PAPI_LIBRARY)
  if(PAPI_INCLUDE_DIRS)
    set(HAVE_PAPI 1)
    mark_as_advanced(HAVE_PAPI)
  endif()
endif()
if(NOT HAVE_PAPI)
  message(FATAL_ERROR, "Could not find PAPI LIBRARY and/or papi.h. Make sure they are correctly installed!")
endif()

#include(FindPackageHandleStandardArgs)
#find_package_handle_standard_args(PAPI DEFAULT_MSG
#    PAPI_LIBRARY
#    PAPI_INCLUDE_DIRS
#)

mark_as_advanced(
    PAPI_PREFIX_DIRS
    PAPI_LIBRARY
    PAPI_INCLUDE_DIRS
)

