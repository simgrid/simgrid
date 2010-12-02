#IF YOU HAVE INSTALL SIMGRID IN A SPECIAL DIRECTORY
#YOU CAN SPECIFY SIMGRID_ROOT OR GRAS_ROOT

# TO CALL THIS FILE USE
	#set(CMAKE_MODULE_PATH 
	#${CMAKE_MODULE_PATH}
	#${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/Modules
	#)

message("GRAS_ROOT	= $ENV{GRAS_ROOT}")
message("SIMGRID_ROOT	= $ENV{SIMGRID_ROOT}")
message("LD_LIBRARY_PATH	= $ENV{LD_LIBRARY_PATH}")

find_library(HAVE_SIMGRID_LIB
    NAME simgrid
    HINTS
    $ENV{LD_LIBRARY_PATH}
    $ENV{GRAS_ROOT}
	$ENV{SIMGRID_ROOT}
    PATH_SUFFIXES lib64 lib
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)

find_path(HAVE_GRAS_H gras.h
    HINTS
    $ENV{GRAS_ROOT}
	$ENV{SIMGRID_ROOT}
    PATH_SUFFIXES include
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)

find_program(HAVE_TESH
	NAMES tesh
	HINTS
	$ENV{GRAS_ROOT}
	$ENV{SIMGRID_ROOT}
	PATH_SUFFIXES bin
	PATHS
	/opt
	/opt/local
	/opt/csw
	/sw
	/usr
)

find_program(HAVE_GRAS_STUB
	NAMES gras_stub_generator
	HINTS
	$ENV{GRAS_ROOT}
	$ENV{SIMGRID_ROOT}
	PATH_SUFFIXES bin
	PATHS
	/opt
	/opt/local
	/opt/csw
	/sw
	/usr
)

message("HAVE_SIMGRID_LIB 	= ${HAVE_SIMGRID_LIB}")
message("HAVE_GRAS_H 		= ${HAVE_GRAS_H}")
message("HAVE_TESH 		= ${HAVE_TESH}")
message("HAVE_GRAS_STUB 		= ${HAVE_GRAS_STUB}")