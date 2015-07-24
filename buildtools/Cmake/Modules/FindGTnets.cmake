find_library(HAVE_GTNETS_LIB
  NAME gtsim
  PATHS ${gtnets_path} ${gtnets_path}/lib
  )
  
if (HAVE_GTNETS_LIB)
  message(STATUS "Looking for GTNetS library - found")
else()
  message(STATUS "Looking for GTNetS library - no found (search path: ${gtnets_path})")
  message(STATUS "  library file name must be libgtsim.so (not gtnets.so, not libgtsim-opt.so)")
endif()

find_path(HAVE_SIMULATOR_H
  NAME simulator.h
  PATH_SUFFIXES include include/gtnets
  PATHS
  ${gtnets_path}
  )
if (HAVE_GTNETS_LIB)
  message(STATUS "Looking for GTNetS header simulator.h - found")
else()
  message(STATUS "Looking for GTNetS header simulator.h - no found")
endif()

string(REPLACE "/libgtnets.${LIB_EXE}" ""  GTNETS_LIB_PATH "${HAVE_GTNETS_LIB}")

if(HAVE_GTNETS_LIB AND HAVE_SIMULATOR_H)

  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -I${HAVE_SIMULATOR_H} -lgtsim -L${GTNETS_LIB_PATH} ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_gtnets.cpp
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
      message(FATAL_ERROR "\nGTNetS library found but unusable. Did you set LD_LIBRARY_PATH?\n\texport LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${GTNETS_LIB_PATH}\n\n")
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