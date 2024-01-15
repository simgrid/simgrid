# CMake find module to search for the SimGrid library.

# Copyright (c) 2016-2024. The SimGrid Team.
#
# This file is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

#
# USERS OF PROGRAMS USING SIMGRID
# -------------------------------
#
# If cmake does not find this file, add its path to CMAKE_PREFIX_PATH:
#    CMAKE_PREFIX_PATH="/path/to/FindSimGrid.cmake:$CMAKE_PREFIX_PATH"  cmake .
#
# If this file does not find SimGrid, define SimGrid_PATH:
#    cmake -DSimGrid_PATH=/path/to/simgrid .

#
# DEVELOPERS OF PROGRAMS USING SIMGRID
# ------------------------------------
#
#  1. Include this file in your own CMakeLists.txt (before defining any target)
#     Either by copying it in your tree, or (recommended) by using the
#     version automatically installed by SimGrid.
#
#  2. This will define a target called 'SimGrid::Simgrid'. Use it as:
#       target_link_libraries(your-simulator SimGrid::SimGrid)
#
#  It also defines a SimGrid_VERSION macro, that you can use to deal with API
#    evolutions as follows:
#
#    #if SimGrid_VERSION < 31800
#      (code to use if the installed version is lower than v3.18)
#    #elif SimGrid_VERSION < 31900
#      (code to use if we are using SimGrid v3.18.x)
#    #else
#      (code to use with SimGrid v3.19+)
#    #endif
#
#  Since SimGrid header files require C++17, so we set CMAKE_CXX_STANDARD to 17.
#    Change this variable in your own file if you need a later standard.

# DEVELOPPERS OF MPI PROGRAMS USING SIMGRID
#   You should use smpi_c_target() on the targets that are intended to run in SMPI.
#   ${SMPIRUN} is correctly set if it's installed.
#
#   Example:
#     add_executable(roundtrip roundtrip.c)
#     smpi_c_target(roundtrip)
#
#     enable_testing()
#     add_test(NAME Roundtrip
#              COMMAND ${SMPIRUN} -platform ${CMAKE_SOURCE_DIR}/../cluster_backbone.xml -np 2 ./roundtrip)

#
# IMPROVING THIS FILE
# -------------------
#  - Use automatic SimGridConfig.cmake creation via export/install(EXPORT in main CMakeLists.txt:
#    https://cliutils.gitlab.io/modern-cmake/chapters/exporting.html
#    https://cmake.org/Wiki/CMake/Tutorials/How_to_create_a_ProjectConfig.cmake_file
#    https://github.com/boostcon/cppnow_presentations_2017/blob/master/05-19-2017_friday/effective_cmake__daniel_pfeifer__cppnow_05-19-2017.pdf

cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_path(SimGrid_INCLUDE_DIR
  NAMES simgrid/config.h
  NAMES simgrid/version.h
  PATHS ${SimGrid_PATH}/include /opt/simgrid/include
)
if (NOT SimGrid_INCLUDE_DIR)
  # search under the old name
  find_path(SimGrid_INCLUDE_DIR
    NAMES simgrid_config.h
    PATHS ${SimGrid_PATH}/include /opt/simgrid/include
  )
endif()
find_library(SimGrid_LIBRARY
  NAMES simgrid
  PATHS ${SimGrid_PATH}/lib /opt/simgrid/lib
)
mark_as_advanced(SimGrid_INCLUDE_DIR)
mark_as_advanced(SimGrid_LIBRARY)

if (SimGrid_INCLUDE_DIR)
  set(SimGrid_VERSION_REGEX "^#define SIMGRID_VERSION_(MAJOR|MINOR|PATCH) ([0-9]+)$")
  if (EXISTS "${SimGrid_INCLUDE_DIR}/simgrid/version.h")
    file(STRINGS "${SimGrid_INCLUDE_DIR}/simgrid/version.h" SimGrid_VERSION_STRING REGEX ${SimGrid_VERSION_REGEX})
  elseif (EXISTS "${SimGrid_INCLUDE_DIR}/simgrid/config.h")
    file(STRINGS "${SimGrid_INCLUDE_DIR}/simgrid/config.h" SimGrid_VERSION_STRING REGEX ${SimGrid_VERSION_REGEX})
  else()
    file(STRINGS "${SimGrid_INCLUDE_DIR}/simgrid_config.h" SimGrid_VERSION_STRING REGEX ${SimGrid_VERSION_REGEX})
  endif()
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

if (SimGrid_FOUND)

  find_program(SMPIRUN smpirun
               HINTS ${SimGrid_PATH}/bin /opt/simgrid/bin)

  MACRO(smpi_c_target NAME)
    target_compile_options(${NAME} PUBLIC "-include;smpi/smpi_helpers.h;-fPIC;-shared;-Wl,-z,defs")
    target_link_options(${NAME} PUBLIC "-fPIC;-shared;-Wl,-z,defs;-lm")
    target_link_libraries(${NAME} PUBLIC ${SimGrid_LIBRARY})
    target_include_directories(${NAME} PUBLIC "${SimGrid_INCLUDE_DIR};${SimGrid_INCLUDE_DIR}/smpi")
  ENDMACRO()

  add_library(SimGrid::SimGrid SHARED IMPORTED)
  set_target_properties(SimGrid::SimGrid PROPERTIES
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${SimGrid_INCLUDE_DIR}
    INTERFACE_COMPILE_FEATURES cxx_alias_templates
    IMPORTED_LOCATION ${SimGrid_LIBRARY}
  )
  # We need C++17, so check for it just in case the user removed it since compiling SimGrid
  if (NOT CMAKE_VERSION VERSION_LESS 3.8)
    # 3.8+ allows us to simply require C++17 (or higher)
    set_property(TARGET SimGrid::SimGrid PROPERTY INTERFACE_COMPILE_FEATURES cxx_std_17)
  else ()
    # Old CMake can't do much. Just check the CXX_FLAGS and inform the user when a C++17 feature does not work
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_CXX_FLAGS}")
    check_cxx_source_compiles("
#if __cplusplus < 201703L
#error
#else
int main(){}
#endif
" _SIMGRID_CXX17_ENABLED)
    if (NOT _SIMGRID_CXX17_ENABLED)
        message(WARNING "C++17 is required to use SimGrid. Enable it with e.g. -std=c++17")
    endif ()
    unset(_SIMGRID_CXX14_ENABLED CACHE)
  endif ()
endif ()

