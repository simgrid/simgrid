#IF YOU HAVE INSTALL SIMGRID LIBRARIES AND SIMGRID BINARIES IN A SPECIAL DIRECTORY
#YOU CAN SPECIFY SIMGRID_LIBRARY_PAT AND SIMGRID_BIN_PATH OR SIMPLY LD_LIBRARY_PATH

# TO CALL THIS FILE USE
	#set(CMAKE_MODULE_PATH 
	#${CMAKE_MODULE_PATH}
	#${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/Modules
	#)

message("SIMGRID_LIBRARY_PATH	= $ENV{SIMGRID_LIBRARY_PATH}")
message("SIMGRID_BIN_PATH	= $ENV{SIMGRID_BIN_PATH}")
message("LD_LIBRARY_PATH	= $ENV{LD_LIBRARY_PATH}")

find_library(HAVE_SIMGRID_LIB
    NAME simgrid
    HINTS
    $ENV{LIBRARIES}
    $ENV{LD_LIBRARY_PATH}
    $ENV{SIMGRID_LIBRARY_PATH}
    PATH_SUFFIXES lib64 lib simgrid/lib64 simgrid/lib
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)

find_path(HAVE_SIMGRID_JAVA_LIB simgrid.jar
    HINTS
    $ENV{LIBRARIES}
    $ENV{LD_LIBRARY_PATH}
    $ENV{SIMGRID_LIBRARY_PATH}
    PATH_SUFFIXES lib64/share lib/share simgrid/lib64/share simgrid/lib/share
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)

find_path(HAVE_GRAS_H gras.h
    HINTS
    $ENV{INCLUDES}
    $ENV{LD_LIBRARY_PATH}
    $ENV{SIMGRID_LIBRARY_PATH}
    PATH_SUFFIXES include simgrid/include
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)

string(REPLACE "/include" "/bin" OPTIONAL_BIN_PATH "HAVE_GRAS_H")

find_program(HAVE_TESH
NAMES tesh
HINTS
$ENV{SIMGRID_BIN_PATH}
$ENV{LD_LIBRARY_PATH}
PATH_SUFFIXES bin simgrid/bin
PATHS
${OPTIONAL_BIN_PATH}
/opt
/opt/local
/opt/csw
/sw
/usr
)

find_program(HAVE_GRAS_STUB
NAMES gras_stub_generator
HINTS
$ENV{SIMGRID_BIN_PATH}
$ENV{LD_LIBRARY_PATH}
PATH_SUFFIXES bin simgrid/bin
PATHS
/opt
/opt/local
/opt/csw
/sw
/usr
)

message("HAVE_SIMGRID_LIB 	= ${HAVE_SIMGRID_LIB}")
message("HAVE_SIMGRID_JAVA_LIB 	= ${HAVE_SIMGRID_JAVA_LIB}")
message("HAVE_GRAS_H 		= ${HAVE_GRAS_H}")
message("HAVE_TESH 		= ${HAVE_TESH}")
message("HAVE_GRAS_STUB 		= ${HAVE_GRAS_STUB}")