if(enable_smpi AND enable_smpi_MPICH3_testsuite AND SMPI_FORTRAN)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  set(CMAKE_INCLUDE_CURRENT_DIR ON)
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  add_executable(infotest2f EXCLUDE_FROM_ALL infotest2f.f)
  add_executable(infotestf  EXCLUDE_FROM_ALL infotestf.f)
  add_dependencies(tests infotestf infotest2f)
  target_link_libraries(infotest2f simgrid mtest_f77)
  target_link_libraries(infotestf simgrid mtest_f77)
endif()

set(examples_src
  ${examples_src}
 ${CMAKE_CURRENT_SOURCE_DIR}/infotest2f.f
 ${CMAKE_CURRENT_SOURCE_DIR}/infotestf.f
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
