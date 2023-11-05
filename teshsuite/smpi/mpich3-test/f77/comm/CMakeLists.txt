if(enable_smpi AND enable_testsuite_smpi_MPICH3 AND SMPI_FORTRAN)
  set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

 # add_executable(commerrf EXCLUDE_FROM_ALL commerrf.f)
  add_executable(commnamef EXCLUDE_FROM_ALL commnamef.f)

  add_dependencies(tests commnamef)
 # add_dependencies(tests commerrf)
 # target_link_libraries(commerrf simgrid mtest_f77)
  target_link_libraries(commnamef simgrid mtest_f77)
endif()

set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/commerrf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/commnamef.f
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
