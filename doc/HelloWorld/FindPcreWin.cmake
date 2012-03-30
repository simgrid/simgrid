# LIB libpcre.dll
find_library(PATH_PCRE_LIB 
	NAMES pcre
    HINTS
    $ENV{SIMGRID_PCRE_LIBRARY_PATH}
    $ENV{PCRE_LIBRARY_PATH}
    PATH_SUFFIXES bin/ GnuWin32/bin
    )
        
find_path(PATH_PCRE_H "pcre.h"
    HINTS
    $ENV{SIMGRID_PCRE_LIBRARY_PATH}
    $ENV{PCRE_LIBRARY_PATH}
    PATH_SUFFIXES include/ GnuWin32/include
    )
    
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
       string(REGEX REPLACE "/pcre.h" "" PATH_PCRE_H "${PATH_PCRE_H}")
	   string(REGEX MATCH "-I${PATH_PCRE_H} " operation "${CMAKE_C_FLAGS}")
	   if(NOT operation)
			SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-I${PATH_PCRE_H} ")
	   endif(NOT operation)
	   string(REGEX REPLACE "/libpcre.dll" "" PATH_PCRE_LIB  "${PATH_PCRE_LIB}")
       link_directories(${PATH_PCRE_LIB})   
else(PATH_PCRE_LIB)
	   message(FATAL_ERROR "Please install the pcre package before using SimGrid.")
endif(PATH_PCRE_LIB AND PATH_PCRE_H)

mark_as_advanced(PATH_PCRE_H)
mark_as_advanced(PATH_PCRE_LIB)