if(enable_smpi AND enable_smpi_MPICH3_testsuite AND SMPI_FORTRAN)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  set(CMAKE_INCLUDE_CURRENT_DIR ON)
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  add_executable(allpairf allpairf.f)
#  add_executable(greqf greqf.f dummyf.f)
#  add_executable(mprobef mprobef.f)
  add_executable(statusesf statusesf.f)

  target_link_libraries(allpairf simgrid mtest_f77)
#  target_link_libraries(greqf simgrid mtest_f77)
  #target_link_libraries(mprobef simgrid mtest_f77)
  target_link_libraries(statusesf simgrid mtest_f77)
endif()

set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/allpairf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/attr1aints.h
  ${CMAKE_CURRENT_SOURCE_DIR}/dummyf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/greqf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/mprobef.f
  ${CMAKE_CURRENT_SOURCE_DIR}/statusesf.f
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
