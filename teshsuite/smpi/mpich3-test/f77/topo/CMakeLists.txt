if(enable_smpi AND enable_smpi_MPICH3_testsuite AND SMPI_FORTRAN)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  add_executable(cartcrf cartcrf.f)
#  add_executable(dgraph_unwgtf dgraph_unwgtf.f)
#  add_executable(dgraph_wgtf dgraph_unwgtf.f)
  target_link_libraries(cartcrf simgrid mtest_f77)
#  target_link_libraries(dgraph_wgtf simgrid mtest_f77)
#  target_link_libraries(dgraph_unwgtf simgrid mtest_f77)
endif()

set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/dgraph_wgtf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/dgraph_unwgtf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/cartcrf.f
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
