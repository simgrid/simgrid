if(enable_smpi AND enable_testsuite_smpi_MPICH3 AND SMPI_FORTRAN)
  set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  #F77 version of the mtest library
  add_library(mtest_f77 STATIC ../util/mtestf.f)
endif()

set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/../util/mtestf.f
  PARENT_SCOPE)
