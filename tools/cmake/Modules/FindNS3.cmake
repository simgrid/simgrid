# Try to find the NS3 library.
#
# The user can hint a path using the NS3_HINT option.
#
# Once done, the following will be defined:
#
#  NS3_INCLUDE_DIRS - the NS3 include directories
#  NS3_LIBRARY_PATH - path to the libs
#  NS3_LIBRARIES - link these to use NS3 (full path to libs)
#
# This could be improved in many ways (patches welcome):
#  - No way to specify a minimal version (v3.10 is hardcoded).
#  - No proper find_package() integration


## ADDING A NS3 VERSION.
#   - Add ns3.${version}-core to the NAME line of the find_library below
#   - Add include/ns3{version} to the PATH_SUFFIXES line of the find_path below

set(HAVE_NS3 0)
set(NS3_HINT ${ns3_path} CACHE PATH "Path to search for NS3 lib and include")

find_library(NS3_LIBRARIES
  NAME ns3-core ns3.14-core ns3.15-core ns3.16-core ns3.17-core ns3.18-core ns3.19-core ns3.20-core ns3.21-core ns3.22-core
  PATH_SUFFIXES lib64 lib ns3/lib
  PATHS
  ${NS3_HINT}
  )

find_path(NS3_INCLUDE_DIR
  NAME ns3/core-module.h
  PATH_SUFFIXES include ns3/include include/ns3.14 include/ns3.15 include/ns3.16 include/ns3.17 include/ns3.18 include/ns3.19 include/ns3.20 include/ns3.21 include/ns3.22
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
    set(HAVE_NS3 1)
    
    string(REGEX REPLACE ".*ns([.0-9]+)-core.*" "\\1" NS3_VERSION "${NS3_LIBRARIES}")
    get_filename_component(NS3_LIBRARY_PATH "${NS3_LIBRARIES}" PATH)

    # Compute NS3_PATH
    string(REGEX REPLACE "(.*)/lib" "\\1" NS3_PATH "${NS3_LIBRARY_PATH}")
    
    message(STATUS "NS-3 found (v3.${NS3_VERSION} in ${NS3_PATH}).")

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

if(HAVE_NS3)
  link_directories(${NS3_LIBRARY_PATH})
  include_directories(${NS3_INCLUDE_DIR})
else()
  message(STATUS "Warning: To use NS-3 Please install ns3 at least version 3.10 (http://www.nsnam.org/releases/)")
endif()
