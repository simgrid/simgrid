cmake_minimum_required(VERSION 2.6)

if(enable_smpi AND enable_smpi_MPICH3_testsuite AND SMPI_F2C)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
  include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../include/")

  add_executable(attrmpi1f attrmpi1f.f ../util/mtestf.f)
  add_executable(baseattr2f baseattr2f.f ../util/mtestf.f)
  add_executable(baseattrf baseattrf.f ../util/mtestf.f)
  add_executable(commattr2f commattr2f.f ../util/mtestf.f)
  add_executable(commattr3f commattr3f.f ../util/mtestf.f)
  add_executable(commattrf commattrf.f  ../util/mtestf.f)
  add_executable(typeattr2f typeattr2f.f  ../util/mtestf.f)
  add_executable(typeattr3f typeattr3f.f  ../util/mtestf.f)
  add_executable(typeattrf typeattrf.f  ../util/mtestf.f)

  target_link_libraries(attrmpi1f  simgrid)
  target_link_libraries(baseattr2f  simgrid)
  target_link_libraries(baseattrf  simgrid)
  target_link_libraries(commattr2f  simgrid)
  target_link_libraries(commattr3f  simgrid)
  target_link_libraries(commattrf  simgrid)
  target_link_libraries(typeattr2f  simgrid)
  target_link_libraries(typeattr3f  simgrid)
  target_link_libraries(typeattrf  simgrid)

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
  ${CMAKE_CURRENT_SOURCE_DIR}/attraints.h
  ${CMAKE_CURRENT_SOURCE_DIR}/attrmpi1f.f
  ${CMAKE_CURRENT_SOURCE_DIR}/baseattr2f.f
  ${CMAKE_CURRENT_SOURCE_DIR}/baseattrf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/commattr2f.f
  ${CMAKE_CURRENT_SOURCE_DIR}/commattr3f.f
  ${CMAKE_CURRENT_SOURCE_DIR}/commattrf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/typeattr2f.f
  ${CMAKE_CURRENT_SOURCE_DIR}/typeattr3f.f
  ${CMAKE_CURRENT_SOURCE_DIR}/typeattrf.f
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
