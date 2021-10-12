# Search for libunwind and components, both includes and libraries
#
# Copyright (C) 2003-2021 The SimGrid Team.
# This is distributed under the LGPL licence but please contact us for
# relicensing if you need. This is merely free software, no matter the licence.
#
#
# Input environment variables:
#    LIBUNWIND_HINT: path to libunwind installation (e.g., /usr)
#                    (only needed for non-standard installs)
#
# You can tune the needed components here.
# TODO: we should take this as a parameter if I knew how to do so.

# SimGrid needs unwind-ptrace on Linux and FreeBSD
if("${CMAKE_SYSTEM}" MATCHES "Linux|FreeBSD")
  set(LIBUNWIND_COMPONENTS ${LIBUNWIND_COMPONENTS} unwind-ptrace unwind-generic)
endif()

#
#  Output variables:
#     HAVE_LIBUNWIND     : if all components were found was found
#     LIBUNWIND_LIBRARIES: List of all libraries to load (-lunwind -lunwind-x86_64 and such)
#
#  Other effects:
#    - Calls include_directories() on where libunwind.h lives
#    - Calls link_directories() on where the libs live

# Of course also need the core lib
set(LIBUNWIND_COMPONENTS ${LIBUNWIND_COMPONENTS} "unwind")

message(STATUS "Looking for libunwind:")
# Let's assume we have it, and invalidate if parts are missing
SET(HAVE_LIBUNWIND 1)

#
# Search for the header file
#

find_path(PATH_LIBUNWIND_H "libunwind.h"
  HINTS
    $ENV{LIBUNWIND_HINT}
    $ENV{LD_LIBRARY_PATH}
  PATH_SUFFIXES include/ GnuWin32/include
  PATHS /opt /opt/local /opt/csw /sw /usr)
if(PATH_LIBUNWIND_H)
  string(REGEX REPLACE "/libunwind.h"               "" PATH_LIBUNWIND_H   "${PATH_LIBUNWIND_H}")
  message("   Found libunwind.h in ${PATH_LIBUNWIND_H}")
  include_directories(${PATH_LIBUNWIND_H})
else()
  message("   NOT FOUND libunwind.h")
  SET(HAVE_LIBUNWIND 0)
endif()
mark_as_advanced(PATH_LIBUNWIND_H)

#
# Search for the library components
#

foreach(component ${LIBUNWIND_COMPONENTS})
  find_library(PATH_LIBUNWIND_LIB_${component}
    NAMES ${component}
    HINTS
      $ENV{LIBUNWIND_HINT}
      $ENV{LD_LIBRARY_PATH}
    PATH_SUFFIXES lib/ GnuWin32/lib lib/system
    PATHS /opt /opt/local /opt/csw /sw /usr /usr/lib/)
  if(PATH_LIBUNWIND_LIB_${component})
    # message("     ${component}  ${PATH_LIBUNWIND_LIB_${component}}")
    string(REGEX REPLACE "/lib${component}.*[.]${LIB_EXE}$" "" PATH_LIBUNWIND_LIB_${component} "${PATH_LIBUNWIND_LIB_${component}}")
    message("   Found lib${component}.${LIB_EXE} in ${PATH_LIBUNWIND_LIB_${component}}")
    link_directories(${PATH_LIBUNWIND_LIB_${component}})

    if(${component} STREQUAL "unwind" AND APPLE)
        # Apple forbids to link directly against its libunwind implementation
        # So let's comply to that stupid restriction and link against the System framework
        SET(LIBUNWIND_LIBRARIES "${LIBUNWIND_LIBRARIES} -lSystem")
    else()
        SET(LIBUNWIND_LIBRARIES "${LIBUNWIND_LIBRARIES} -l${component}")
    endif()

  else()
    message("   Looking for lib${component}.${LIB_EXE} - not found")
    SET(HAVE_LIBUNWIND 0)
  endif()
  mark_as_advanced(PATH_LIBUNWIND_LIB_${component})
endforeach()
unset(component)
unset(LIBUNWIND_COMPONENTS)

#
# Conclude and cleanup
#
if(HAVE_LIBUNWIND)
  message(STATUS "Dependencies induced by libunwind: ${LIBUNWIND_LIBRARIES}")
else()
  message(STATUS "Some libunwind components are missing")
  set(LIBUNWIND_LIBRARIES "")
endif()
