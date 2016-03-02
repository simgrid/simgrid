if(enable_smpi AND enable_smpi_MPICH3_testsuite)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  add_executable(infodel infodel.c)
  add_executable(infodup infodup.c)
#  add_executable(infoenv infoenv.c)
  add_executable(infomany2 infomany2.c)
  add_executable(infomany infomany.c)
  add_executable(infoorder infoorder.c)
  add_executable(infotest infotest.c)
  add_executable(infovallen infovallen.c)

  target_link_libraries(infodel simgrid mtest_c)
  target_link_libraries(infodup simgrid mtest_c)
#  target_link_libraries(infoenv simgrid mtest_c)
  target_link_libraries(infomany2 simgrid mtest_c)
  target_link_libraries(infomany simgrid mtest_c)
  target_link_libraries(infoorder simgrid mtest_c)
  target_link_libraries(infotest simgrid mtest_c)
  target_link_libraries(infovallen simgrid mtest_c)
endif()

set(examples_src
  ${examples_src}
 ${CMAKE_CURRENT_SOURCE_DIR}/infodel.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/infodup.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/infoenv.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/infomany2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/infomany.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/infoorder.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/infotest.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/infovallen.c 
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
