find_library(PATH_LIBDW_LIB
  NAMES dw
  HINTS
  $ENV{SIMGRID_LIBDW_LIBRARY_PATH}
  $ENV{LD_LIBRARY_PATH}
  $ENV{LIBDW_LIBRARY_PATH}
  PATH_SUFFIXES lib/ GnuWin32/lib
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr)


find_path(PATH_LIBDW_H "elfutils/libdw.h"
  HINTS
  $ENV{SIMGRID_LIBDW_LIBRARY_PATH}
  $ENV{LD_LIBRARY_PATH}
  $ENV{LIBDW_LIBRARY_PATH}
  PATH_SUFFIXES include/ GnuWin32/include
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr)

message(STATUS "Looking for libdw.h")
if(PATH_LIBDW_H)
  message(STATUS "Looking for libdw.h - found")
else()
  message(STATUS "Looking for libdw.h - not found")
endif()

message(STATUS "Looking for libdw")
if(PATH_LIBDW_LIB)
  message(STATUS "Looking for libdw - found")
else()
  message(STATUS "Looking for libdw - not found")
endif()

if(PATH_LIBDW_LIB AND PATH_LIBDW_H)
  string(REGEX REPLACE "/libdw.*[.]${LIB_EXE}$" "" PATH_LIBDW_LIB "${PATH_LIBDW_LIB}")
  string(REGEX REPLACE "/libdw.h"               "" PATH_LIBDW_H   "${PATH_LIBDW_H}")

  include_directories(${PATH_LIBDW_H})
  link_directories(${PATH_LIBDW_LIB})

else()
  message(FATAL_ERROR "Please either install the libdw-dev package (or equivalent) or turn off the model-checking option of SimGrid.")
endif()

mark_as_advanced(PATH_LIBDW_H)
mark_as_advanced(PATH_LIBDW_LIB)
