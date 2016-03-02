if(enable_smpi AND enable_smpi_MPICH3_testsuite AND SMPI_FORTRAN)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpif90")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  #F90 version of the mtest library
  add_library(mtest_f90 STATIC ../util/mtestf90.f90)
endif()

set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/../util/mtestf90.f90
  PARENT_SCOPE)
