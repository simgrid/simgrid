cmake_minimum_required(VERSION 2.6)

if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
  set(MPICH_FLAGS "-DHAVE_STDLIB_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STRING_H=1 -DUSE_STDARG=1 -DHAVE_LONG_DOUBLE=1 -DHAVE_PROTOTYPES=1 -DHAVE_SIGNAL_H=1 -DHAVE_SIGACTION=1 -DHAVE_SLEEP=1 -DHAVE_SYSCONF=1  -Wno-error=unused-variable")
  include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_BINARY_DIR}/../include/")


  add_executable(groupcreate groupcreate.c ../util/mtest.c)
  add_executable(groupnullincl groupnullincl.c ../util/mtest.c)
  add_executable(grouptest2 grouptest2.c ../util/mtest.c)
  add_executable(grouptest grouptest.c ../util/mtest.c)
  add_executable(gtranks gtranks.c ../util/mtest.c)
  add_executable(gtranksperf gtranksperf.c ../util/mtest.c)



  target_link_libraries(groupcreate  simgrid)
  target_link_libraries(groupnullincl  simgrid)
  target_link_libraries(grouptest2  simgrid)
  target_link_libraries(grouptest  simgrid)
  target_link_libraries(gtranks  simgrid)
  target_link_libraries(gtranksperf  simgrid)



 set_target_properties(groupcreate PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(groupnullincl PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(grouptest2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(grouptest PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(gtranks PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(gtranksperf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")

endif()

set(tesh_files
  ${tesh_files}
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
 ${CMAKE_CURRENT_SOURCE_DIR}/groupcreate.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/groupnullincl.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/grouptest2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/grouptest.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/gtranks.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/gtranksperf.c 
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/runtests
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE
  )
