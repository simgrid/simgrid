find_library(HAVE_NS3_LIB
    NAME ns3
    PATH_SUFFIXES lib64 lib ns3/lib
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
    ${ns3_path}
)

find_library(HAVE_NS3_CORE_LIB
    NAME ns3-core
    PATH_SUFFIXES lib64 lib ns3/lib
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
    ${ns3_path}
)

find_path(HAVE_CORE_MODULE_H
	NAME ns3/core-module.h
    PATH_SUFFIXES include ns3/include
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
    ${ns3_path}
)

message(STATUS "Looking for core-module.h")
if(HAVE_CORE_MODULE_H)
message(STATUS "Looking for core-module.h - found")
else(HAVE_CORE_MODULE_H)
message(STATUS "Looking for core-module.h - not found")
endif(HAVE_CORE_MODULE_H)
mark_as_advanced(HAVE_CORE_MODULE_H)

message(STATUS "Looking for lib ns3")
if(HAVE_NS3_LIB)
message(STATUS "Looking for lib ns3 - found")
else(HAVE_NS3_LIB)
message(STATUS "Looking for lib ns3 - not found")
endif(HAVE_NS3_LIB)
mark_as_advanced(HAVE_NS3_LIB)

message(STATUS "Looking for lib ns3-core")
if(HAVE_NS3_CORE_LIB)
message(STATUS "Looking for lib ns3-core - found")
else(HAVE_NS3_CORE_LIB)
message(STATUS "Looking for lib ns3-core - not found")
endif(HAVE_NS3_CORE_LIB)
mark_as_advanced(HAVE_NS3_LIB)
mark_as_advanced(HAVE_NS3_CORE_LIB)

if(HAVE_CORE_MODULE_H)
    if(HAVE_NS3_LIB)
        message(STATUS "Warning: NS-3 version <= 3.10")
        set(HAVE_NS3 1)
        set(NS3_VERSION 310)
        string(REPLACE "/libns3.${LIB_EXE}" ""  HAVE_NS3_LIB "${HAVE_NS3_LIB}")
    endif(HAVE_NS3_LIB)
    if(HAVE_NS3_CORE_LIB)
        message(STATUS "NS-3 version > 3.10")
        set(HAVE_NS3 1)
        set(NS3_VERSION 312)
        string(REPLACE "/libns3-core.${LIB_EXE}" ""  HAVE_NS3_LIB "${HAVE_NS3_CORE_LIB}")
    endif(HAVE_NS3_CORE_LIB)
endif(HAVE_CORE_MODULE_H)

if(HAVE_NS3)
	string(REGEX MATCH "${HAVE_NS3_LIB}" operation "$ENV{LD_LIBRARY_PATH}")
	if(NOT operation)
		message(STATUS "Warning: To use NS-3 don't forget to set LD_LIBRARY_PATH with:	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${HAVE_NS3_LIB}")
	else(NOT operation)
	
		string(REGEX MATCH "-L${HAVE_NS3_LIB} " operation1 "${CMAKE_C_FLAGS}")
		if(NOT operation1)
			SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-L${HAVE_NS3_LIB} ")
		endif(NOT operation1)
		
		string(REGEX MATCH "-I${HAVE_CORE_MODULE_H} " operation1 "${CMAKE_C_FLAGS}")
		if(NOT operation1)
			SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-I${HAVE_CORE_MODULE_H} ")
		endif(NOT operation1)
	
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-I${HAVE_CORE_MODULE_H} -L${HAVE_NS3_LIB} ")
	endif(NOT operation)		
else(HAVE_NS3)
    message(STATUS "Warning: To use NS-3 Please install ns3 at least version 3.10 (http://www.nsnam.org/releases/)")
endif(HAVE_NS3)

if(HAVE_NS3 AND enable_supernovae)
    set(enable_supernovae OFF)
endif(HAVE_NS3 AND enable_supernovae)
