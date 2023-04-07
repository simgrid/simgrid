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
#  - No proper find_package() integration

set(SIMGRID_HAVE_NS3 0)
find_package(PkgConfig)

pkg_check_modules(NS3 ns3-core>=3.28 ns3-csma ns3-point-to-point ns3-internet ns3-network ns3-applications ns3-wifi)
if(NS3_FOUND) # Starting from 3.36, ns3 provides a working pkg-config file, making things much easier
  set(SIMGRID_HAVE_NS3 1)
  set(NS3_LIBRARY_PATH ${NS3_LIBRARY_DIRS})
  set(NS3_INCLUDE_DIR ${NS3_INCLUDE_DIRS})  
  set(NS3_LIBRARIES "")
  foreach(elm ${NS3_LDFLAGS})
    if (elm MATCHES "^-L" OR elm MATCHES "^-lns3")
      if ((NOT NS3_LIBRARIES MATCHES " ${elm} ") AND (NOT NS3_LIBRARIES MATCHES " ${elm}$")) 
        set(NS3_LIBRARIES "${NS3_LIBRARIES} ${elm}")
      endif()
    endif()
  endforeach()

  set(NS3_VERSION "${NS3_ns3-core_VERSION}")
  string(REGEX REPLACE "3\\.([-.0-9a-z]+)" "\\1" NS3_MINOR_VERSION "${NS3_VERSION}")
  if(NS3_MINOR_VERSION MATCHES "\\.")
    string(REGEX REPLACE "^[0-9]*\\.([0-9]+$)" "\\1" NS3_PATCH_VERSION "${NS3_MINOR_VERSION}")
    string(REGEX REPLACE "^([0-9]+)\\.[0-9]*$" "\\1" NS3_MINOR_VERSION "${NS3_MINOR_VERSION}")
  else()
    set(NS3_PATCH_VERSION "0")
  endif()

  
  # No pkg-config found. Try to go the old path
else()
  set(NS3_HINT ${ns3_path} CACHE PATH "Path to search for NS3 lib and include")

  set(NS3_KNOWN_VERSIONS "3-dev" "3.28" "3.29" "3.30" "3.31" "3.32" "3.33" "3.34" "3.35") # subsequent versions use pkg-config

  foreach (_ns3_ver ${NS3_KNOWN_VERSIONS})
    list(APPEND _ns3_LIB_SEARCH_DIRS "ns${_ns3_ver}-core" "ns${_ns3_ver}-core-optimized" "ns${_ns3_ver}-core-debug" "ns${_ns3_ver}-core-default")
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
      elseif(NS3_LIBRARIES MATCHES "-default")
        set (NS3_SUFFIX "-default")
      else()
        set (NS3_SUFFIX "")
      endif()
      message(STATUS "ns-3 found ${NS3_LIBRARIES}")
      string(REGEX REPLACE ".*libns(.*)-core.*" "\\1" NS3_VERSION "${NS3_LIBRARIES}")
      string(REGEX REPLACE "3\\.([-.0-9a-z]+)" "\\1" NS3_MINOR_VERSION "${NS3_VERSION}")
      if(NS3_MINOR_VERSION MATCHES "dev")
        set(NS3_MINOR_VERSION "99")
      endif()
      if(NS3_MINOR_VERSION MATCHES "\\.")
        string(REGEX REPLACE "^[0-9]*\\.([0-9]+$)" "\\1" NS3_PATCH_VERSION "${NS3_MINOR_VERSION}")
        string(REGEX REPLACE "^([0-9]+)\\.[0-9]*$" "\\1" NS3_MINOR_VERSION "${NS3_MINOR_VERSION}")
      else()
        set(NS3_PATCH_VERSION "0")
      endif()
      get_filename_component(NS3_LIBRARY_PATH "${NS3_LIBRARIES}" PATH)

      # Compute NS3_PATH
      string(REGEX REPLACE "(.*)/lib" "\\1" NS3_PATH "${NS3_LIBRARY_PATH}")

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

  set(NS3_LIBRARIES "")
  foreach(lib core csma point-to-point internet network applications wifi)
    set(NS3_LIBRARIES "${NS3_LIBRARIES} -lns${NS3_VERSION}-${lib}${NS3_SUFFIX}")
  endforeach()
endif()

set(SIMGRID_HAVE_NS3_GetNextEventTime FALSE)
if(SIMGRID_HAVE_NS3)
  try_compile(compile_ns3 ${CMAKE_BINARY_DIR} ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_ns3.cpp
              CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${NS3_INCLUDE_DIR}" "-DLINK_DIRECTORIES=${NS3_LIBRARY_PATH}"
              LINK_LIBRARIES "${NS3_LIBRARIES}"
              OUTPUT_VARIABLE compile_ns3_output)
  if(NOT compile_ns3)
    message(STATUS "ns-3 does not have the ns3::Simulator::GetNextEventTime patch. The ns-3 model will not be idempotent. Compilation output:\n ${compile_ns3_output}")
  else()
    set(SIMGRID_HAVE_NS3_GetNextEventTime TRUE)
  endif()
  file(REMOVE ${CMAKE_BINARY_DIR}/prog_ns3)
  message(STATUS "ns-3 found (v${NS3_VERSION}; minor ver:${NS3_MINOR_VERSION}; patch ver:${NS3_PATCH_VERSION}; GetNextEventTime patch: ${SIMGRID_HAVE_NS3_GetNextEventTime}; libpath: ${NS3_LIBRARY_PATH}).")
  link_directories(${NS3_LIBRARY_PATH})
  include_directories(${NS3_INCLUDE_DIR})
else()
  message(STATUS "Warning: Please install ns-3 (version 3.22 or higher -- http://www.nsnam.org/releases/) or disable this feature.")
endif()
