cmake_minimum_required(VERSION 2.6)

if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
  include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

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
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE
  )
