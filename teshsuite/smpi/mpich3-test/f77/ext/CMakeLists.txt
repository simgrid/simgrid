cmake_minimum_required(VERSION 2.6)

if(enable_smpi AND SMPI_F2C)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
  include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  add_executable(add1size add1size.h ../util/mtestf.f)
#  add_executable(allocmemf allocmemf.f ../util/mtestf.f)
#  add_executable(c2f2cf c2f2cf.f c2f2c.c ../util/mtestf.f)
#  add_executable(ctypesinf ctypesinf.f ctypesfromc.c ../util/mtestf.f)

  target_link_libraries(add1size  simgrid)
#  target_link_libraries(allocmemf  simgrid)
#  target_link_libraries(c2f2cf  simgrid)
#  target_link_libraries(ctypesinf  simgrid)

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
  ${CMAKE_CURRENT_SOURCE_DIR}/add1size.h
  ${CMAKE_CURRENT_SOURCE_DIR}/allocmemf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/c2f2c.c
  ${CMAKE_CURRENT_SOURCE_DIR}/c2f2cf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/c2fmult.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ctypesfromc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ctypesinf.f
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
