find_path(PATH_LIBSIGC++_H "sigc++/sigc++.h"
  HINTS
  $ENV{LD_LIBRARY_PATH}
  PATH_SUFFIXES include/sigc++-2.0/ include/
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr)

find_path(PATH_LIBSIGC++CONFIG_H "sigc++config.h"
  HINTS
  $ENV{LD_LIBRARY_PATH}
  PATH_SUFFIXES lib/x86_64-linux-gnu/sigc++-2.0/include/ include/
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr)

find_library(PATH_LIBSIGC++_LIB
  NAMES sigc-2.0
  HINTS
  $ENV{LD_LIBRARY_PATH}
  PATH_SUFFIXES lib/x86_64-linux-gnu/ lib/sigc++/ lib/
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr)

message(STATUS "Looking for sigc++/sigc++.h")
if(PATH_LIBSIGC++_H)
  message(STATUS "Looking for sigc++/sigc++.h - found")
else()
  message(STATUS "Looking for sigc++/sigc++.h - not found")
endif()

message(STATUS "Looking for sigc++config.h")
if(PATH_LIBSIGC++CONFIG_H)
  message(STATUS "Looking for sigc++config.h - found")
else()
  message(STATUS "Looking for sigc++config.h - not found")
endif()

message(STATUS "Looking for libsigc++")
if(PATH_LIBSIGC++_LIB)
  message(STATUS "Looking for libsigc++ - found")
else()
  message(STATUS "Looking for libsigc++ - not found")
endif()

if(PATH_LIBSIGC++_LIB AND PATH_LIBSIGC++_H AND PATH_LIBSIGC++CONFIG_H)
  string(REGEX REPLACE "/sigc\\+\\+/sigc\\+\\+.h" "" PATH_LIBSIGC++_H   "${PATH_LIBSIGC++_H}")
  string(REGEX REPLACE "/sigc\\+\\+config.h" "" PATH_LIBSIGC++CONFIG_H   "${PATH_LIBSIGC++CONFIG_H}")
  string(REGEX REPLACE "/libsig.*" "" PATH_LIBSIGC++_LIB "${PATH_LIBSIGC++_LIB}")
      
  include_directories(${PATH_LIBSIGC++_H})
  include_directories(${PATH_LIBSIGC++CONFIG_H})
  link_directories(${PATH_LIBSIGC++_LIB})
  set(SIMGRID_HAVE_LIBSIG "1")
else()
  set(SIMGRID_HAVE_LIBSIG "0")
endif()

mark_as_advanced(PATH_LIBSIGC++_H)
mark_as_advanced(PATH_LIBSIGC++CONFIG_H)
mark_as_advanced(PATH_LIBSIGC++_LIB)
