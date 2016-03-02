if(enable_smpi AND enable_smpi_MPICH3_testsuite)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

#  add_executable(attrself attrself.c)
  add_executable(exitst1 exitst1.c)
  add_executable(exitst2 exitst2.c)
#  add_executable(exitst3 exitst3.c)
  add_executable(finalized finalized.c)
  add_executable(initstat initstat.c)
  add_executable(library_version library_version.c)
#  add_executable(timeout timeout.c)
  add_executable(version version.c)

#  target_link_libraries(attrself simgrid mtest_c)
  target_link_libraries(exitst1 simgrid mtest_c)
  target_link_libraries(exitst2 simgrid mtest_c)
#  target_link_libraries(exitst3 simgrid mtest_c)
  target_link_libraries(finalized simgrid mtest_c)
  target_link_libraries(initstat simgrid mtest_c)
  target_link_libraries(library_version simgrid mtest_c)
#  target_link_libraries(timeout simgrid mtest_c)
  target_link_libraries(version simgrid mtest_c)
endif()

set(examples_src
  ${examples_src}
 ${CMAKE_CURRENT_SOURCE_DIR}/attrself.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/exitst1.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/exitst2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/exitst3.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/finalized.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/initstat.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/library_version.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/timeout.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/version.c 
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
