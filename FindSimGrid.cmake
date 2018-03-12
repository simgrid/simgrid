# CMake find module to search for the SimGrid library. 

# Copyright (c) 2016-2018. The SimGrid Team.
#
# This file is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

# Once done, the following will be defined:
#
#  CMake >= 2.8.12:
#    Target SimGrid::Simgrid
#
#    Use as:
#      target_link_libraries(your-simulator SimGrid::SimGrid)
#
#  Older CMake (< 2.8.12)
#    SimGrid_INCLUDE_DIR - the SimGrid include directories
#    SimGrid_LIBRARY - link your simulator against it to use SimGrid
#
#    Use as:
#      include_directories("${SimGrid_INCLUDE_DIR}" SYSTEM)
#      target_link_libraries(your-simulator ${SimGrid_LIBRARY})
#
# This could be improved:
#  - Use automatic SimGridConfig.cmake creation via export/install(EXPORT in main CMakeLists.txt:
#    https://cliutils.gitlab.io/modern-cmake/chapters/exporting.html
#    https://cmake.org/Wiki/CMake/Tutorials/How_to_create_a_ProjectConfig.cmake_file
#    https://github.com/boostcon/cppnow_presentations_2017/blob/master/05-19-2017_friday/effective_cmake__daniel_pfeifer__cppnow_05-19-2017.pdf

cmake_minimum_required(VERSION 2.8)

find_path(SimGrid_INCLUDE_DIR
  NAMES simgrid_config.h
  PATHS /opt/simgrid/include
)
find_library(SimGrid_LIBRARY
  NAMES simgrid
  PATHS /opt/simgrid/lib
)
mark_as_advanced(SimGrid_INCLUDE_DIR)
mark_as_advanced(SimGrid_LIBRARY)

if (SimGrid_INCLUDE_DIR)
  set(SimGrid_VERSION_REGEX "^#define SIMGRID_VERSION_(MAJOR|MINOR|PATCH) ([0-9]+)$")
  file(STRINGS "${SimGrid_INCLUDE_DIR}/simgrid_config.h" SimGrid_VERSION_STRING REGEX ${SimGrid_VERSION_REGEX})
  set(SimGrid_VERSION "")

  # Concat the matches to MAJOR.MINOR.PATCH assuming they appear in this order
  foreach(match ${SimGrid_VERSION_STRING})
    if(SimGrid_VERSION)
      set(SimGrid_VERSION "${SimGrid_VERSION}.")
    endif()
    string(REGEX REPLACE ${SimGrid_VERSION_REGEX} "${SimGrid_VERSION}\\2" SimGrid_VERSION ${match})
    set(SimGrid_VERSION_${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
  endforeach()
  unset(SimGrid_VERSION_STRING)
  unset(SimGrid_VERSION_REGEX)  
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SimGrid
  FOUND_VAR SimGrid_FOUND
  REQUIRED_VARS SimGrid_INCLUDE_DIR SimGrid_LIBRARY
  VERSION_VAR SimGrid_VERSION
)

if (SimGrid_FOUND AND NOT CMAKE_VERSION VERSION_LESS 2.8.12)
  add_library(SimGrid::SimGrid SHARED IMPORTED)
  set_target_properties(SimGrid::SimGrid PROPERTIES
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${SimGrid_INCLUDE_DIR}
    INTERFACE_COMPILE_FEATURES cxx_alias_templates
    IMPORTED_LOCATION ${SimGrid_LIBRARY}
  )
  # We need C++11, so check for it
  if (NOT CMAKE_VERSION VERSION_LESS 3.8)
    # 3.8+ allows us to simply require C++11 (or higher)
    set_property(TARGET SimGrid::SimGrid PROPERTY INTERFACE_COMPILE_FEATURES cxx_std_11)
  elseif (NOT CMAKE_VERSION VERSION_LESS 3.1)
    # 3.1+ is similar but for certain features. We pick just one
    set_property(TARGET SimGrid::SimGrid PROPERTY INTERFACE_COMPILE_FEATURES cxx_alias_templates)
  else ()
    # Old CMake can't do much. Just check the CXX_FLAGS and inform the user when a C++11 feature does not work
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_CXX_FLAGS}")
    check_cxx_source_compiles("using Foo = int; int main(){}" _SIMGRID_CXX11_ENABLED)
    if (NOT _SIMGRID_CXX11_ENABLED)
        message(WARNING "C++11 is required to use this library. Enable it with e.g. -std=c++11")
    endif ()
    unset(_SIMGRID_CXX11_ENABLED CACHE)
  endif ()
endif ()

