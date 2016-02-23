if(enable_smpi AND enable_smpi_MPICH3_testsuite)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  add_executable(commcreatep commcreatep.c)
#  add_executable(dtpack dtpack.c)
#  add_executable(indexperf indexperf.c)
#  add_executable(manyrma manyrma.c)
#  add_executable(nestvec2 nestvec2.c)
#  add_executable(nestvec nestvec.c)
  add_executable(non_zero_root non_zero_root.c)
  add_executable(sendrecvl sendrecvl.c)
  add_executable(timer timer.c)
  add_executable(transp-datatype transp-datatype.c)
  add_executable(twovec twovec.c)

  target_link_libraries(commcreatep simgrid mtest_c)
#  target_link_libraries(dtpack simgrid mtest_c)
#  target_link_libraries(indexperf simgrid mtest_c)
#  target_link_libraries(manyrma simgrid mtest_c)
#  target_link_libraries(nestvec2 simgrid mtest_c)
#  target_link_libraries(nestvec simgrid mtest_c)
  target_link_libraries(non_zero_root simgrid mtest_c)
  target_link_libraries(sendrecvl simgrid mtest_c)
  target_link_libraries(timer simgrid mtest_c)
  target_link_libraries(transp-datatype simgrid mtest_c)
  target_link_libraries(twovec simgrid mtest_c)
endif()

set(examples_src
  ${examples_src}
 ${CMAKE_CURRENT_SOURCE_DIR}/allredtrace.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/commcreatep.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/dtpack.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/indexperf.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/manyrma.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/nestvec2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/nestvec.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/non_zero_root.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/sendrecvl.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/timer.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/transp-datatype.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/twovec.c 
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/README
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
