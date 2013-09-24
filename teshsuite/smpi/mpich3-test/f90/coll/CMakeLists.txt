cmake_minimum_required(VERSION 2.6)

if(enable_smpi AND enable_smpi_MPICH3_testsuite AND SMPI_F2C)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpif90")
  endif()

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
  include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")

  add_executable(allredint8f90 allredint8f90.f90 ../util/mtestf90.f90)
  add_executable(allredopttf90 allredopttf90.f90 ../util/mtestf90.f90)
  add_executable(alltoallvf90 alltoallvf90.f90 ../util/mtestf90.f90)
  add_executable(alltoallwf90 alltoallwf90.f90 ../util/mtestf90.f90)
  add_executable(exscanf90 exscanf90.f90 ../util/mtestf90.f90)
  add_executable(inplacef90 inplacef90.f90 ../util/mtestf90.f90)
 # add_executable(nonblockingf90 nonblockingf90.f90 ../util/mtestf90.f90)
 # add_executable(nonblocking_inpf90 nonblocking_inpf90.f90 ../util/mtestf90.f90)
  add_executable(red_scat_blockf90 red_scat_blockf90.f90 ../util/mtestf90.f90)
  add_executable(redscatf90 redscatf90.f90 ../util/mtestf90.f90)
  add_executable(reducelocalf90 reducelocalf90.f90 ../util/mtestf90.f90)
  add_executable(split_typef90 split_typef90.f90 ../util/mtestf90.f90)
  add_executable(uallreducef90 uallreducef90.f90 ../util/mtestf90.f90)
  add_executable(vw_inplacef90 vw_inplacef90.f90 ../util/mtestf90.f90)
  target_link_libraries(allredint8f90  simgrid)
  target_link_libraries(allredopttf90  simgrid)
  target_link_libraries(alltoallvf90  simgrid)
  target_link_libraries(alltoallwf90  simgrid)
  target_link_libraries(exscanf90  simgrid)
  target_link_libraries(inplacef90  simgrid)
 # target_link_libraries(nonblockingf90  simgrid)
 # target_link_libraries(nonblocking_inpf90  simgrid)
  target_link_libraries(red_scat_blockf90  simgrid)
  target_link_libraries(redscatf90  simgrid)
  target_link_libraries(reducelocalf90  simgrid)
  target_link_libraries(split_typef90  simgrid)
  target_link_libraries(uallreducef90  simgrid)
  target_link_libraries(vw_inplacef90  simgrid)

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
  ${CMAKE_CURRENT_SOURCE_DIR}/allredint8f90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/allredopttf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/alltoallvf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/alltoallwf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/exscanf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/inplacef90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/nonblockingf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/nonblocking_inpf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/red_scat_blockf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/redscatf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/reducelocalf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/split_typef90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/uallreducef90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/vw_inplacef90.f90
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/../util/mtestf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE
  )
