find_path(HAVE_F2C_H f2c.h
    HINTS
    $ENV{LD_LIBRARY_PATH}
    PATH_SUFFIXES include/
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)

find_program(F2C_EXE 
	NAME f2c
    PATH_SUFFIXES bin/
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)

find_library(HAVE_F2C_LIB
    NAME f2c
    HINTS
    $ENV{LD_LIBRARY_PATH}
    PATH_SUFFIXES lib/
    PATHS
    /opt
    /opt/local
    /opt/csw
    /sw
    /usr
)

if(HAVE_F2C_H)
set(HAVE_SMPI_F2C_H 1)
endif(HAVE_F2C_H)

mark_as_advanced(HAVE_F2C_H)
mark_as_advanced(F2C_EXE)
mark_as_advanced(HAVE_F2C_LIB)