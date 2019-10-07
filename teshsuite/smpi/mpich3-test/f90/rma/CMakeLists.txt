if(enable_smpi AND enable_smpi_MPICH3_testsuite AND SMPI_FORTRAN)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpif90")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
 
  foreach(test winaccf90
             # winerrf90
           winfencef90 wingroupf90
           baseattrwinf90 winattr2f90 winattrf90
           wingetf90 winnamef90 winscale1f90
         # winscale2f90
           )
    add_executable(${test} EXCLUDE_FROM_ALL ${test}.f90)
    add_dependencies(tests ${test})
    target_link_libraries(${test} simgrid mtest_f90)	
  endforeach()

  add_executable(c2f2cwinf90 EXCLUDE_FROM_ALL c2f2cwinf90.f90 c2f902cwin.c)
  add_dependencies(tests c2f2cwinf90)
  target_link_libraries(c2f2cwinf90 simgrid mtest_f90)
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
