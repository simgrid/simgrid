# Cmake macro to search for the SimGrid library. 

# Copyright (c) 2016. The SimGrid Team (version 20161210).
#
# This file is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

# The user can hint a path using the SimGrid_PATH option.
#
# Once done, the following will be defined:
#  
#  SimGrid_INCLUDE_DIR - the SimGrid include directories
#  SimGrid_LIBRARY - link your simulator against it to use SimGrid
#
# Afterward, you should probably do the following:
#
#  include_directories("${SimGrid_INCLUDE_DIR}" SYSTEM)
#  target_link_libraries(your-simulator ${SimGrid_LIBRARY})
#
# This could be improved in many ways (patches welcome):
#  - Not finding SimGrid is fatal. But who would want to 
#    overcome such a desperate situation anyway? ;)
#  - No way to specify a minimal version
#  - No proper find_package() integration

set(SimGrid_PATH ${SimGrid_PATH} CACHE PATH "Path to SimGrid library and include")
find_path(SimGrid_INCLUDE_DIR
  NAMES simgrid_config.h
  HINTS ${SimGrid_PATH}/include /usr/include /usr/local/include /opt/simgrid/include
)
find_library(SimGrid_LIBRARY
  NAMES simgrid
  HINTS ${SimGrid_PATH}/lib /usr/lib /usr/local/lib /opt/simgrid/lib
)
mark_as_advanced(SimGrid_INCLUDE_DIR)
mark_as_advanced(SimGrid_LIBRARY)

if (NOT EXISTS "${SimGrid_INCLUDE_DIR}")
  message(FATAL_ERROR "Unable to find SimGrid header file. Please point the SimGrid_PATH variable to the installation directory.")
endif ()
if (NOT EXISTS "${SimGrid_LIBRARY}")
  message(FATAL_ERROR "Unable to find SimGrid library. Please point the SimGrid_PATH variable to the installation directory.")
endif ()

# Extract the actual path
if (NOT "$SimGrid_INCLUDE_DIR" STREQUAL "${SimGrid_PATH}/include")
      string(REGEX REPLACE "(.*)/include" "\\1" SimGrid_PATH "${SimGrid_INCLUDE_DIR}")
endif()

# Extract the actual version number
file(READ "${SimGrid_INCLUDE_DIR}/simgrid_config.h" _SimGrid_CONFIG_H_CONTENTS)
set(_SimGrid_VERSION_REGEX ".*#define SIMGRID_VERSION_STRING \"SimGrid version ([^\"]*)\".*")
if ("${_SimGrid_CONFIG_H_CONTENTS}" MATCHES "${_SimGrid_VERSION_REGEX}")
      string(REGEX REPLACE "${_SimGrid_VERSION_REGEX}" "\\1" SimGrid_VERSION "${_SimGrid_CONFIG_H_CONTENTS}")
endif ("${_SimGrid_CONFIG_H_CONTENTS}" MATCHES "${_SimGrid_VERSION_REGEX}")
  

message(STATUS "SimGrid found in ${SimGrid_PATH} (version ${SimGrid_VERSION})")


