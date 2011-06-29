### SET THE LIBRARY EXTENSION AND GCC VERSION
if(APPLE) #MAC
	set(LIB_EXE "dylib")
else(APPLE)
    if(WIN32) #WINDOWS
        set(LIB_EXE "a")
        set(BIN_EXE ".exe")
    else(WIN32) #UNIX
	    set(LIB_EXE "so")
    endif(WIN32)
endif(APPLE)

find_library(PATH_PCRE_LIB 
	NAMES pcre
    HINTS
    $ENV{LD_LIBRARY_PATH}
    $ENV{PCRE_LIBRARY_PATH}
    PATH_SUFFIXES lib/ GnuWin32/lib
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr)
    
find_path(PATH_PCRE_H "pcre.h"
    HINTS
    $ENV{LD_LIBRARY_PATH}
    $ENV{PCRE_LIBRARY_PATH}
    PATH_SUFFIXES include/ GnuWin32/include
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr)
    
set(HAVE_PCRE_LIB 0)

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
    set(HAVE_PCRE_LIB 1)
else(PATH_PCRE_LIB)
    message(FATAL_ERROR "Please install the libpcre3-dev package or equivalent before using it.")
endif(PATH_PCRE_LIB AND PATH_PCRE_H)
    
mark_as_advanced(PATH_PCRE_H)
mark_as_advanced(PATH_PCRE_LIB)