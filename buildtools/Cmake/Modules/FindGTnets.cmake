find_library(HAVE_GTNETS_LIB
  NAME gtnets
  PATH_SUFFIXES lib64 lib lib64/gtnets lib/gtnets
  PATHS
  ${gtnets_path}
  )

find_path(HAVE_SIMULATOR_H
  NAME simulator.h
  PATH_SUFFIXES include include/gtnets
  PATHS
  ${gtnets_path}
  )

string(REPLACE "/libgtnets.${LIB_EXE}" ""  GTNETS_LIB_PATH "${HAVE_GTNETS_LIB}")

if(HAVE_GTNETS_LIB AND HAVE_SIMULATOR_H)

  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -I${HAVE_SIMULATOR_H} -lgtnets -L${GTNETS_LIB_PATH} ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_gtnets.cpp
    OUTPUT_VARIABLE COMPILE_GTNETS_VAR)
  if(COMPILE_GTNETS_VAR)
    SET(HAVE_GTNETS 0)
  else()
    SET(HAVE_GTNETS 1)

    string(REGEX MATCH "-L${GTNETS_LIB_PATH} " operation "${CMAKE_C_FLAGS}")
    if(NOT operation)
      SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-L${GTNETS_LIB_PATH} ")
    endif()

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-I${HAVE_SIMULATOR_H} -L${GTNETS_LIB_PATH} ")

    string(REGEX MATCH "${GTNETS_LIB_PATH}" operation "$ENV{LD_LIBRARY_PATH}")
    if(NOT operation)
      message(FATAL_ERROR "\n\nTo use GTNETS don't forget to set LD_LIBRARY_PATH with \n\texport LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${GTNETS_LIB_PATH}\n\n")
    endif()

  endif()

else()
  if(NOT HAVE_GTNETS_LIB)
    message(STATUS "Gtnets is enabled but can't find it.")
  endif()
  if(NOT HAVE_SIMULATOR_H)
    message(STATUS "Gtnets needs simulator.h")
  endif()
endif()

message(STATUS "Looking for lib gtnets patch")
if(HAVE_GTNETS)
  message(STATUS "Looking for lib gtnets patch - found")
else()
  message(STATUS "Looking for lib gtnets patch - not found")
endif()

mark_as_advanced(HAVE_GTNETS_LIB)
mark_as_advanced(HAVE_SIMULATOR_H)