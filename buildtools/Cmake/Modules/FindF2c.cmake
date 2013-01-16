find_path(HAVE_F2C_H f2c.h
  HINTS
  $ENV{LD_LIBRARY_PATH}
  PATH_SUFFIXES include/
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr
  )

find_program(F2C_EXE
  NAME f2c
  PATH_SUFFIXES bin/
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr
  )

find_library(HAVE_F2C_LIB
  NAME f2c
  HINTS
  $ENV{LD_LIBRARY_PATH}
  PATH_SUFFIXES lib/
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr
  )

if(HAVE_F2C_H)
  set(HAVE_SMPI_F2C_H 1)
endif()

message(STATUS "Looking for f2c.h")
if(HAVE_F2C_H)
  message(STATUS "Looking for f2c.h - found")
else()
  message(STATUS "Looking for f2c.h - not found")
endif()

message(STATUS "Looking for lib f2c")
if(HAVE_F2C_LIB)
  message(STATUS "Looking for lib f2c - found")
else()
  message(STATUS "Looking for lib f2c - not found")
endif()

message(STATUS "Looking for bin f2c")
if(F2C_EXE)
  message(STATUS "Found F2C: ${F2C_EXE}")
else()
  message(STATUS "Looking for bin f2c - not found")
endif()

mark_as_advanced(HAVE_F2C_H)
mark_as_advanced(F2C_EXE)
mark_as_advanced(HAVE_F2C_LIB)

if(HAVE_F2C_H)
  include_directories(${HAVE_F2C_H})
else()
    message("-- Fortran for smpi is not installed (http://www.netlib.org/f2c/).")
endif()


#Some old versions of 64 bits systems seem to have a different size between C and Fortran Datatypes
#Deactivate F2C with these versions, in order to avoid breaking test cases in legacy systems (as Fedora 13)
set(COMPILER_OK 1)
if(PROCESSOR_x86_64 AND "${CMAKE_SYSTEM}" MATCHES "Linux" AND ${CMAKE_C_COMPILER_ID} STREQUAL "GNU" AND "4.5" STRGREATER "${COMPILER_C_VERSION_MAJOR_MINOR}" )
    set(COMPILER_OK 0)
    message("Your C compiler is a bit old and Fortran support is quite problematic on 64 bit platforms, F2C has been deactivated")
endif()

set(SMPI_F2C 0)
if(HAVE_F2C_H AND F2C_EXE AND HAVE_F2C_LIB AND COMPILER_OK)
  set(SMPI_F2C 1)
endif()
