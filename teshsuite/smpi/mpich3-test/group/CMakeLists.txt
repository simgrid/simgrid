if(enable_smpi AND enable_smpi_MPICH3_testsuite)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  add_executable(groupcreate groupcreate.c)
#  add_executable(groupnullincl groupnullincl.c)
  add_executable(grouptest2 grouptest2.c)
  add_executable(grouptest grouptest.c)
  add_executable(gtranks gtranks.c)
  add_executable(gtranksperf gtranksperf.c)

  target_link_libraries(groupcreate simgrid mtest_c)
#  target_link_libraries(groupnullincl simgrid mtest_c)
  target_link_libraries(grouptest2 simgrid mtest_c)
  target_link_libraries(grouptest simgrid mtest_c)
  target_link_libraries(gtranks simgrid mtest_c)
  target_link_libraries(gtranksperf simgrid mtest_c)
endif()

set(examples_src
  ${examples_src}
 ${CMAKE_CURRENT_SOURCE_DIR}/groupcreate.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/groupnullincl.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/grouptest2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/grouptest.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/gtranks.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/gtranksperf.c 
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
