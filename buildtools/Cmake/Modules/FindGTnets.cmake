find_path(HAVE_RNGSTREAM_H RngStream.h
    HINTS
    $ENV{LD_LIBRARY_PATH}
    PATH_SUFFIXES include include/gtnets
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)

find_library(HAVE_GTNETS_LIB
    NAME gtnets
    HINTS
    $ENV{LD_LIBRARY_PATH}
    PATH_SUFFIXES lib64 lib lib64/gtnets lib/gtnets
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)

string(REPLACE "/libgtnets.${LIB_EXE}" ""  GTNETS_LIB_PATH "${HAVE_GTNETS_LIB}")

if(HAVE_GTNETS_LIB AND HAVE_RNGSTREAM_H)

	exec_program("${CMAKE_CXX_COMPILER} -I${HAVE_RNGSTREAM_H} -lgtnets -L${GTNETS_LIB_PATH} ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_gtnets.cpp " OUTPUT_VARIABLE COMPILE_GTNETS_VAR)	
	if(COMPILE_GTNETS_VAR)
		SET(HAVE_GTNETS 0)
	else(COMPILE_GTNETS_VAR)
		SET(HAVE_GTNETS 1)	
		
		string(REGEX MATCH "-L${GTNETS_LIB_PATH} " operation "${CMAKE_C_FLAGS}")
		if(NOT operation)
			SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-L${GTNETS_LIB_PATH} ")
		endif(NOT operation)
		
		string(REGEX MATCH "-I${HAVE_RNGSTREAM_H} " operation "${CMAKE_C_FLAGS}")
		if(NOT operation)
			SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-I${HAVE_RNGSTREAM_H} ")
		endif(NOT operation)
		
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-I${HAVE_RNGSTREAM_H} -L${GTNETS_LIB_PATH} ")
		
		string(REGEX MATCH "${GTNETS_LIB_PATH}" operation "$ENV{LD_LIBRARY_PATH}")
		if(NOT operation)
			message(FATAL_ERROR "\n\nTo use GTNETS don't forget to set LD_LIBRARY_PATH with \n\texport LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${GTNETS_LIB_PATH}\n\n")
		endif(NOT operation)
		
	endif(COMPILE_GTNETS_VAR)

else(HAVE_GTNETS_LIB AND HAVE_RNGSTREAM_H)
message(STATUS "Gtnets is enable but can't find it. You should set LD_LIBRARY_PATH...")
endif(HAVE_GTNETS_LIB AND HAVE_RNGSTREAM_H)

message(STATUS "Looking for RngStream.h")
if(HAVE_RNGSTREAM_H)
message(STATUS "Looking for RngStream.h - found")
else(HAVE_RNGSTREAM_H)
message(STATUS "Looking for RngStream.h - not found")
endif(HAVE_RNGSTREAM_H)

message(STATUS "Looking for lib gtnets patch")
if(HAVE_GTNETS)
message(STATUS "Looking for lib gtnets patch - found")
else(HAVE_GTNETS)
message(STATUS "Looking for lib gtnets patch - not found")
endif(HAVE_GTNETS)

mark_as_advanced(HAVE_GTNETS_LIB)
mark_as_advanced(HAVE_RNGSTREAM_H)