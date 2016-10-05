if(enable_smpi AND enable_smpi_MPICH3_testsuite AND SMPI_FORTRAN)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  add_executable(winaccf winaccf.f)
#  add_executable(winerrf winerrf.f)
  add_executable(winfencef winfencef.f)
#  add_executable(wingroupf wingroupf.f)
#  add_executable(baseattrwinf baseattrwinf.f)
#  add_executable(winattr2f winattr2f.f)
#  add_executable(winattrf winattrf.f)
  add_executable(c2f2cwinf c2f2cwinf.f c2f2cwin.c)
  add_executable(wingetf wingetf.f)
  add_executable(winnamef winnamef.f)
  add_executable(winscale1f winscale1f.f)
#  add_executable(winscale2f winscale2f.f)

target_link_libraries(winaccf simgrid mtest_f77)
#target_link_libraries(winerrf simgrid mtest_f77)
target_link_libraries(winfencef simgrid mtest_f77)
#target_link_libraries(wingroupf simgrid mtest_f77)
#target_link_libraries(baseattrwinf simgrid mtest_f77)
target_link_libraries(c2f2cwinf simgrid mtest_f77)
#target_link_libraries(winattr2f simgrid mtest_f77)
#target_link_libraries(winattrf simgrid mtest_f77)
target_link_libraries(wingetf simgrid mtest_f77)
target_link_libraries(winnamef simgrid mtest_f77)
target_link_libraries(winscale1f simgrid mtest_f77)
#target_link_libraries(winscale2f simgrid mtest_f77)
endif()

set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/winaccf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/winerrf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/winfencef.f
  ${CMAKE_CURRENT_SOURCE_DIR}/wingroupf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/baseattrwinf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/c2f2cwin.c
  ${CMAKE_CURRENT_SOURCE_DIR}/c2f2cwinf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  ${CMAKE_CURRENT_SOURCE_DIR}/winattr2f.f
  ${CMAKE_CURRENT_SOURCE_DIR}/winattrf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/wingetf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/winnamef.f
  ${CMAKE_CURRENT_SOURCE_DIR}/winscale1f.f
  ${CMAKE_CURRENT_SOURCE_DIR}/winscale2f.f
  ${CMAKE_CURRENT_SOURCE_DIR}/addsize.h
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
