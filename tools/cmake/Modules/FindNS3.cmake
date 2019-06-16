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

find_library(NS3_LIBRARIES
  NAME ns3-core
       ns3.22-core ns3.22-core-optimized ns3.22-core-debug
       ns3.23-core ns3.23-core-optimized ns3.23-core-debug
       ns3.24-core ns3.24-core-optimized ns3.24-core-debug
       ns3.25-core ns3.25-core-optimized ns3.25-core-debug
       ns3.26-core ns3.26-core-optimized ns3.26-core-debug
       ns3.27-core ns3.27-core-optimized ns3.27-core-debug
       ns3.28-core ns3.28-core-optimized ns3.28-core-debug
       ns3.29-core ns3.29-core-optimized ns3.29-core-debug
       ns3.30-core ns3.30-core-optimized ns3.30-core-debug
       ns3.31-core ns3.31-core-optimized ns3.31-core-debug
       ns3.32-core ns3.32-core-optimized ns3.32-core-debug
       ns3.33-core ns3.33-core-optimized ns3.33-core-debug
       ns3.34-core ns3.34-core-optimized ns3.34-core-debug
  PATH_SUFFIXES lib64 lib ns3/lib
  PATHS
  ${NS3_HINT}
  )

find_path(NS3_INCLUDE_DIR
  NAME ns3/core-module.h
  PATH_SUFFIXES include ns3/include 
                include/ns3.22 include/ns3.23 include/ns3.24 include/ns3.25 include/ns3.26 include/ns3.27 include/ns3.28 include/ns3.29 include/ns3.30
                include/ns3.31 include/ns3.32 include/ns3.33 include/ns3.34
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
