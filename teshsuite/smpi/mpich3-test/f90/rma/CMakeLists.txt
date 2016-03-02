if(enable_smpi AND enable_smpi_MPICH3_testsuite AND SMPI_FORTRAN)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpif90")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  add_executable(winaccf90 winaccf90.f90)
#  add_executable(winerrf90 winerrf90.f90)
  add_executable(winfencef90 winfencef90.f90)
#  add_executable(wingroupf90 wingroupf90.f90)
#  add_executable(baseattrwinf90 baseattrwinf90.f90)
#  add_executable(winattr2f90 winattr2f90.f90)
#  add_executable(winattrf90 winattrf90.f90)
  add_executable(c2f2cwinf90 c2f2cwinf90.f90 c2f902cwin.c)
  add_executable(wingetf90 wingetf90.f90)
  add_executable(winnamef90 winnamef90.f90)
  add_executable(winscale1f90 winscale1f90.f90)
#  add_executable(winscale2f90 winscale2f90.f90)

target_link_libraries(winaccf90 simgrid mtest_f90)
#target_link_libraries(winerrf90 simgrid mtest_f90)
target_link_libraries(winfencef90 simgrid mtest_f90)
#target_link_libraries(wingroupf90 simgrid mtest_f90)
#target_link_libraries(baseattrwinf90 simgrid mtest_f90)
target_link_libraries(c2f2cwinf90 simgrid mtest_f90)
#target_link_libraries(winattr2f90 simgrid mtest_f90)
#target_link_libraries(winattrf90 simgrid mtest_f90)
target_link_libraries(wingetf90 simgrid mtest_f90)
target_link_libraries(winnamef90 simgrid mtest_f90)
target_link_libraries(winscale1f90 simgrid mtest_f90)
#target_link_libraries(winscale2f90 simgrid mtest_f90)
endif()

set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/winaccf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/winerrf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/winfencef90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/wingroupf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/baseattrwinf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/c2f902cwin.c
  ${CMAKE_CURRENT_SOURCE_DIR}/c2f2cwinf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  ${CMAKE_CURRENT_SOURCE_DIR}/winattr2f90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/winattrf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/wingetf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/winnamef90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/winscale1f90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/winscale2f90.f90
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
