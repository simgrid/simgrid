find_program(GFORTRAN_EXE
  NAME gfortran
  PATH_SUFFIXES bin/
  PATHS
  /opt
  /opt/local
  /opt/csw
  /sw
  /usr
  )

mark_as_advanced(GFORTRAN_EXE)

message(STATUS "Looking for bin gfortran")
if(GFORTRAN_EXE)
  message(STATUS "Found gfortran: ${GFORTRAN_EXE}")
else()
  message(STATUS "Looking for bin gfortran - not found")
endif()

set(SMPI_F90 0)
if(GFORTRAN_EXE)
  if(NOT SMPI_F2C)
    message("-- Fortran 90 support for smpi also needs f2c.")
  #elseif(HAVE_MC)
  #  message("-- Fortran 90 support for smpi is currently not compatible with model checking.")
  else()
    set(SMPI_F90 1)
  endif()
endif()

if(NOT SMPI_F90)
  message("-- Fortran 90 support for smpi is disabled.")
endif()
