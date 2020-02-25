if(enable_smpi AND enable_smpi_MPICH3_testsuite AND SMPI_FORTRAN)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  foreach(test winaccf
           winfencef wingroupf
           baseattrwinf winattr2f winattrf
           wingetf winnamef winscale1f
         # winscale2f winerrf
           )
    add_executable(${test} EXCLUDE_FROM_ALL ${test}.f)
    add_dependencies(tests ${test})
    target_link_libraries(${test} simgrid mtest_f77)
  endforeach()
  
  add_executable(c2f2cwinf EXCLUDE_FROM_ALL c2f2cwinf.f c2f2cwin.c)
  set_property(TARGET c2f2cwinf PROPERTY INTERPROCEDURAL_OPTIMIZATION FALSE)
  add_dependencies(tests c2f2cwinf)
  target_link_libraries(c2f2cwinf simgrid mtest_f77)
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
  ${CMAKE_CURRENT_SOURCE_DIR}/attraints.h
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
