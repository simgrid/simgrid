# Try to find the ns-3 library.
#
# The user can hint a path using the NS3_HINT option.
#
# Once done, the following will be defined:
#
#  NS3_INCLUDE_DIRS - the ns-3 include directories
#  NS3_LIBRARY_PATH - path to the libs
#  NS3_LIBRARIES - link these to use ns-3 (full path to libs)
#
# This could be improved in many ways (patches welcome):
#  - No way to specify a minimal version (v3.26 is hardcoded).
#  - No proper find_package() integration

## ADDING A ns-3 VERSION.
#   - Add "ns3.${version}-core ns3.${version}-core-debug ns3.${version}-core-optimized" to the NAME line of the find_library below
#   - Add "include/ns3{version}" to the PATH_SUFFIXES line of the find_path below

set(SIMGRID_HAVE_NS3 0)
set(NS3_HINT ${ns3_path} CACHE PATH "Path to search for NS3 lib and include")

set(NS3_KNOWN_VERSIONS "3.22" "3.23" "3.24" "3.25" "3.26" "3.27" "3.28" "3.29" "3.30" "3.31" "3.32" "3.33" "3.34" "3.35" "3.36" "3.37" "3.38" "3.39" "3.40")

foreach (_ns3_ver ${NS3_KNOWN_VERSIONS})
  list(APPEND _ns3_LIB_SEARCH_DIRS "ns${_ns3_ver}-core" "ns${_ns3_ver}-core-optimized" "ns${_ns3_ver}-core-debug")
  list(APPEND _ns3_INCLUDE_SEARCH_DIRS "include/ns${_ns3_ver}")
endforeach()

find_library(NS3_LIBRARIES
  NAME ns3-core
      ${_ns3_LIB_SEARCH_DIRS}
  PATH_SUFFIXES lib64 lib ns3/lib
  PATHS
  ${NS3_HINT}
  )

find_path(NS3_INCLUDE_DIR
  NAME ns3/core-module.h
  PATH_SUFFIXES include ns3/include
                ${_ns3_INCLUDE_SEARCH_DIRS}
  PATHS
  ${NS3_HINT}
  )

if(NS3_INCLUDE_DIR)
  message(STATUS "Looking for ns3/core-module.h - found")
else()
  message(STATUS "Looking for ns3/core-module.h - not found")
endif()
mark_as_advanced(NS3_INCLUDE_DIR)

message(STATUS "Looking for lib ns3-core")
if(NS3_LIBRARIES)
  message(STATUS "Looking for lib ns3-core - found")
else()
  message(STATUS "Looking for lib ns3-core - not found")
endif()
mark_as_advanced(NS3_LIBRARIES)

if(NS3_INCLUDE_DIR)
  if(NS3_LIBRARIES)
    set(SIMGRID_HAVE_NS3 1)
    if(NS3_LIBRARIES MATCHES "-optimized")
      set (NS3_SUFFIX "-optimized")
    elseif(NS3_LIBRARIES MATCHES "-debug")
      set (NS3_SUFFIX "-debug")
    else()
      set (NS3_SUFFIX "")
    endif()

    string(REGEX REPLACE ".*ns([.0-9]+)-core.*" "\\1" NS3_VERSION "${NS3_LIBRARIES}")
    string(REGEX REPLACE "3.([.0-9]+)" "\\1" NS3_MINOR_VERSION "${NS3_VERSION}")
    get_filename_component(NS3_LIBRARY_PATH "${NS3_LIBRARIES}" PATH)

    # Compute NS3_PATH
    string(REGEX REPLACE "(.*)/lib" "\\1" NS3_PATH "${NS3_LIBRARY_PATH}")

    message(STATUS "ns-3 found (v${NS3_VERSION}; incl:${NS3_INCLUDE_DIR}; lib: ${NS3_LIBRARY_PATH}).")

    if (NOT NS3_LIBRARY_PATH STREQUAL "/usr/lib")
      string(REGEX MATCH "${NS3_LIBRARY_PATH}" MatchResult "$ENV{LD_LIBRARY_PATH}")
      if(NOT MatchResult)
        message(STATUS "Warning: NS3 not installed in system path, and not listed in LD_LIBRARY_PATH."
                       "         You want to: export LD_LIBRARY_PATH=${NS3_LIBRARY_PATH}\${LD_LIBRARY_PATH:+:\$LD_LIBRARY_PATH}")
      endif()
    endif()
  endif()
endif()
mark_as_advanced(NS3_LIBRARY_PATH)

if(SIMGRID_HAVE_NS3)
  link_directories(${NS3_LIBRARY_PATH})
  include_directories(${NS3_INCLUDE_DIR})
else()
  message(STATUS "Warning: Please install ns-3 (version 3.22 or higher -- http://www.nsnam.org/releases/) or disable this feature.")
endif()
