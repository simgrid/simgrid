if(enable_smpi AND enable_smpi_MPICH3_testsuite AND SMPI_FORTRAN)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpif90")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  add_executable(allpairf90 allpairf90.f90)
#  add_executable(greqf90 greqf90.f90 dummyf90.f90)
#  add_executable(mprobef90 mprobef90.f90)
  add_executable(statusesf90 statusesf90.f90)
  target_link_libraries(allpairf90 simgrid mtest_f90)
#  target_link_libraries(greqf90 simgrid mtest_f90)
#  target_link_libraries(mprobef90 simgrid mtest_f90)
  target_link_libraries(statusesf90 simgrid mtest_f90)
endif()

set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/allpairf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/dummyf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/greqf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/mprobef90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/statusesf90.f90
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
