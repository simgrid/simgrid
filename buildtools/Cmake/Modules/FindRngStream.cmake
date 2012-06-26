find_path(HAVE_RNGSTREAM_H
  NAME RngStream.h
  HINTS
  $ENV{HOME}
  PATH_SUFFIXES include
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr
  )

find_library(HAVE_RNGSTREAM_LIB
  NAME rngstreams
  HINTS
  $ENV{HOME}
  PATH_SUFFIXES lib64 lib lib32
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr
  )

message(STATUS "Looking for RngStream.h")
if(HAVE_RNGSTREAM_H)
  message(STATUS "Looking for RngStream.h - found")
  string(REGEX MATCH "-I${HAVE_RNGSTREAM_H} " operation "${CMAKE_C_FLAGS}")
  if(NOT operation)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-I${HAVE_RNGSTREAM_H} ")
  endif(NOT operation)
else(HAVE_RNGSTREAM_H)
  message(STATUS "Looking for RngStream.h - not found")
endif(HAVE_RNGSTREAM_H)

message(STATUS "Looking for lib rngstreams")
if(HAVE_RNGSTREAM_LIB)
  message(STATUS "Looking for lib rngstreams - found")
  string(REGEX REPLACE "/librngstreams.*" "" HAVE_RNGSTREAM_LIB "${HAVE_RNGSTREAM_LIB}")
  string(REGEX MATCH "-L${HAVE_RNGSTREAM_LIB} " operation "${CMAKE_C_FLAGS}")
  if(NOT operation)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-L${HAVE_RNGSTREAM_LIB} ")
  endif(NOT operation)
else(HAVE_RNGSTREAM_LIB)
  message(STATUS "Looking for lib rngstreams - not found")
endif(HAVE_RNGSTREAM_LIB)

mark_as_advanced(HAVE_RNGSTREAM_LIB)
mark_as_advanced(HAVE_RNGSTREAM_H)