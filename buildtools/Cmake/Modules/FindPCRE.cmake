find_library(PATH_PCRE_LIB 
	NAMES pcre
    HINTS
    $ENV{SIMGRID_PCRE_LIBRARY_PATH}
    $ENV{LD_LIBRARY_PATH}
    $ENV{PCRE_LIBRARY_PATH}
    PATH_SUFFIXES lib/ GnuWin32/lib
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr)
    
string(REGEX MATCH ".dll.a" operation "${PATH_PCRE_LIB}")

if(NOT operation)
    if(WIN32)
           set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-DPCRE_STATIC ")
    endif(WIN32)
endif(NOT operation)


find_path(PATH_PCRE_H "pcre.h"
    HINTS
    $ENV{SIMGRID_PCRE_LIBRARY_PATH}
    $ENV{LD_LIBRARY_PATH}
    $ENV{PCRE_LIBRARY_PATH}
    PATH_SUFFIXES include/ GnuWin32/include
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr)

message(STATUS "Looking for pcre.h")
if(PATH_PCRE_H)
message(STATUS "Looking for pcre.h - found")
else(PATH_PCRE_H)
message(STATUS "Looking for pcre.h - not found")
endif(PATH_PCRE_H)

message(STATUS "Looking for lib pcre")
if(PATH_PCRE_LIB)
message(STATUS "Looking for lib pcre - found")
else(PATH_PCRE_LIB)
message(STATUS "Looking for lib pcre - not found")
endif(PATH_PCRE_LIB)

if(WIN32)
    find_path(PATH_PCRE_LICENCE "LICENCE"
        HINTS
        $ENV{SIMGRID_PCRE_LIBRARY_PATH}
        $ENV{LD_LIBRARY_PATH}
        $ENV{PCRE_LIBRARY_PATH}
        PATH_SUFFIXES GnuWin32
        PATHS
        /opt
        /opt/local
        /opt/csw
        /sw
        /usr)
    message(STATUS "Looking for pcre licence")
    if(PATH_PCRE_LICENCE)
    message(STATUS "Looking for pcre licence - found")
    else(PATH_PCRE_LICENCE)
    message(STATUS "Looking for pcre licence - not found")
    endif(PATH_PCRE_LICENCE)
endif(WIN32)

if(PATH_PCRE_LIB AND PATH_PCRE_H)
       string(REGEX REPLACE "/libpcre.*[.]${LIB_EXE}$" "" PATHLIBPCRE "${PATH_PCRE_LIB}")
       string(REGEX REPLACE "/pcre.h" "" PATH_PCRE_H "${PATH_PCRE_H}")
       string(REGEX MATCH "-L${PATHLIBPCRE} " operation "${CMAKE_C_FLAGS}")
	   if(NOT operation)
			SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-L${PATHLIBPCRE} ")
	   endif(NOT operation)
	   string(REGEX MATCH "-I${PATH_PCRE_H} " operation "${CMAKE_C_FLAGS}")
	   if(NOT operation)
			SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-I${PATH_PCRE_H} ")
	   endif(NOT operation)	   
else(PATH_PCRE_LIB)
	   message(FATAL_ERROR "Please install the libpcre3-dev package or equivalent before using SimGrid.")
endif(PATH_PCRE_LIB AND PATH_PCRE_H)

set(PCRE_LIBRARY_PATH $ENV{PCRE_LIBRARY_PATH})

mark_as_advanced(PATH_PCRE_H)
mark_as_advanced(PATH_PCRE_LIB)