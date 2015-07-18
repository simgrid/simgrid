
## ADDING A NS3 VERSION.
#   - Add ns3.${version}-core to the NAME line of the find_library below
#   - Add include/ns3{version} to the PATH_SUFFIXES line of the find_path below

find_library(HAVE_NS3_CORE_LIB
  NAME ns3-core ns3.14-core ns3.15-core ns3.16-core ns3.17-core ns3.18-core ns3.19-core ns3.20-core ns3.21-core ns3.22-core
  PATH_SUFFIXES lib64 lib ns3/lib
  PATHS
  ${ns3_path}
  )

find_path(HAVE_CORE_MODULE_H
  NAME ns3/core-module.h
  PATH_SUFFIXES include ns3/include include/ns3.14 include/ns3.15 include/ns3.16 include/ns3.17 include/ns3.18 include/ns3.19 include/ns3.20 include/ns3.21 include/ns3.22
  PATHS
  ${ns3_path}
  )



if(HAVE_CORE_MODULE_H)
  message(STATUS "Looking for ns3/core-module.h - found")
else()
  message(STATUS "Looking for ns3/core-module.h - not found")
endif()
mark_as_advanced(HAVE_CORE_MODULE_H)

message(STATUS "Looking for lib ns3-core")
if(HAVE_NS3_CORE_LIB)
  message(STATUS "Looking for lib ns3-core - found")
else()
  message(STATUS "Looking for lib ns3-core - not found")
endif()

mark_as_advanced(HAVE_NS3_LIB)
mark_as_advanced(HAVE_NS3_CORE_LIB)

if(HAVE_CORE_MODULE_H)
  if(HAVE_NS3_LIB)
    message(STATUS "Warning: NS-3 version <= 3.10")
    set(HAVE_NS3 1)
    set(NS3_VERSION_MINOR 10)
    get_filename_component(NS3_LIBRARY_PATH "${HAVE_NS3_LIB}" PATH)
  endif()
  if(HAVE_NS3_CORE_LIB)
    message(STATUS "NS-3 version > 3.10")
    string(REGEX REPLACE ".*ns3.([0-9]+)-core.*" "\\1" NS3_VERSION_MINOR "${HAVE_NS3_CORE_LIB}")
    set(HAVE_NS3 1)
    get_filename_component(NS3_LIBRARY_PATH "${HAVE_NS3_CORE_LIB}" PATH)
  endif()
endif()

if(HAVE_NS3)
  string(REGEX MATCH "${NS3_LIBRARY_PATH}" operation "$ENV{LD_LIBRARY_PATH}")
  if(NOT operation)
    message(STATUS "Warning: To use NS-3 don't forget to set LD_LIBRARY_PATH with: export LD_LIBRARY_PATH=${NS3_LIBRARY_PATH}\${LD_LIBRARY_PATH:+:\$LD_LIBRARY_PATH}")
  endif()

  link_directories(${NS3_LIBRARY_PATH})
  include_directories(${HAVE_CORE_MODULE_H})
else()
  message(STATUS "Warning: To use NS-3 Please install ns3 at least version 3.10 (http://www.nsnam.org/releases/)")
endif()
