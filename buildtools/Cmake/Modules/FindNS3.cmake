# If you use NS-3 version 3.14 (prefer used at most 3.13) be sure having do
# ln -sf libns3.14.1-applications-debug.so libns3-applications.so
# ln -sf libns3.14.1-internet-debug.so libns3-internet.so
# ln -sf libns3.14.1-point-to-point-debug.so libns3-point-to-point.so
# ln -sf libns3.14.1-csma-debug.so libns3-csma.so
# ln -sf libns3.14.1-core-debug.so libns3-core.so

find_library(HAVE_NS3_LIB
  NAME ns3
  PATH_SUFFIXES lib64 lib ns3/lib
  PATHS
  ${ns3_path}
  )

find_library(HAVE_NS3_CORE_LIB
  NAME ns3-core ns3.14-core ns3.17-core
  PATH_SUFFIXES lib64 lib ns3/lib
  PATHS
  ${ns3_path}
  )

find_path(HAVE_CORE_MODULE_H
  NAME ns3/core-module.h
  PATH_SUFFIXES include ns3/include include/ns3.14.1 include/ns3.17
  PATHS
  ${ns3_path}
  )

message(STATUS "Looking for core-module.h")
if(HAVE_CORE_MODULE_H)
  message(STATUS "Looking for core-module.h - found")
else()
  message(STATUS "Looking for core-module.h - not found")
endif()
mark_as_advanced(HAVE_CORE_MODULE_H)

message(STATUS "Looking for lib ns3")
if(HAVE_NS3_LIB)
  message(STATUS "Looking for lib ns3 - found")
else()
  message(STATUS "Looking for lib ns3 - not found")
endif()
mark_as_advanced(HAVE_NS3_LIB)

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
