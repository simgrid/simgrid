if(enable_smpi AND enable_testsuite_smpi_MPICH3 AND SMPI_FORTRAN)
  set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpif90")

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  #F90 version of the mtest library
  add_library(mtest_f90 STATIC ../util/mtestf90.f90)
  #fake dependency, to ensure mpi.mod is generated properly before
  target_link_libraries(mtest_f90 mpi)
endif()

set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/../util/mtestf90.f90
  PARENT_SCOPE)
