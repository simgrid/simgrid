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

  add_executable(attrself attrself.c ../util/mtest.c)
  add_executable(exitst1 exitst1.c ../util/mtest.c)
  add_executable(exitst2 exitst2.c ../util/mtest.c)
  add_executable(exitst3 exitst3.c ../util/mtest.c)
  add_executable(finalized finalized.c ../util/mtest.c)
  add_executable(initstat initstat.c ../util/mtest.c)
  add_executable(library_version library_version.c ../util/mtest.c)
  add_executable(timeout timeout.c ../util/mtest.c)
  add_executable(version version.c ../util/mtest.c)

  target_link_libraries(attrself  simgrid)
  target_link_libraries(exitst1  simgrid)
  target_link_libraries(exitst2  simgrid)
  target_link_libraries(exitst3  simgrid)
  target_link_libraries(finalized  simgrid)
  target_link_libraries(initstat  simgrid)
  target_link_libraries(library_version  simgrid)
  target_link_libraries(timeout  simgrid)
  target_link_libraries(version  simgrid)

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
 ${CMAKE_CURRENT_SOURCE_DIR}/attrself.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/exitst1.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/exitst2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/exitst3.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/finalized.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/initstat.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/library_version.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/timeout.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/version.c 
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
