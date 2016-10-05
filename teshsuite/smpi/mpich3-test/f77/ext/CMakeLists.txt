if(enable_smpi AND enable_smpi_MPICH3_testsuite AND SMPI_FORTRAN)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

#  add_executable(allocmemf allocmemf.f)
#  add_executable(c2f2cf c2f2cf.f c2f2c.c)
#  add_executable(ctypesinf ctypesinf.f ctypesfromc.c)

#  target_link_libraries(allocmemf simgrid mtest_f77)
#  target_link_libraries(c2f2cf simgrid mtest_f77)
#  target_link_libraries(ctypesinf simgrid mtest_f77)
endif()

set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/add1size.h
  ${CMAKE_CURRENT_SOURCE_DIR}/allocmemf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/c2f2c.c
  ${CMAKE_CURRENT_SOURCE_DIR}/c2f2cf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/c2fmult.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ctypesfromc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ctypesinf.f
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
