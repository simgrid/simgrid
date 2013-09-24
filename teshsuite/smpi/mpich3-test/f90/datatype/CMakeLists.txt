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

  add_executable(allctypesf90 allctypesf90.f90 ../util/mtestf90.f90)
 # add_executable(createf90 createf90.f90 ../util/mtestf90.f90)
  add_executable(gaddressf90 gaddressf90.f90 ../util/mtestf90.f90)
 # add_executable(get_elem_d get_elem_d.f90 ../util/mtestf90.f90)
 # add_executable(get_elem_u get_elem_u.f90 ../util/mtestf90.f90)
  add_executable(hindex1f90 hindex1f90.f90 ../util/mtestf90.f90)
  add_executable(hindexed_blockf90 hindexed_blockf90.f90 ../util/mtestf90.f90)
  add_executable(indtype indtype.f90 ../util/mtestf90.f90)
  add_executable(kinds kinds.f90 ../util/mtestf90.f90)
  add_executable(packef90 packef90.f90 ../util/mtestf90.f90)
 # add_executable(sizeof sizeof.f90 ../util/mtestf90.f90)
 # add_executable(structf structf.f90 ../util/mtestf90.f90)
 # add_executable(trf90 trf90.f90 ../util/mtestf90.f90)
  add_executable(typecntsf90 typecntsf90.f90 ../util/mtestf90.f90)
  add_executable(typem2f90 typem2f90.f90 ../util/mtestf90.f90)
  add_executable(typename3f90 typename3f90.f90 ../util/mtestf90.f90)
  add_executable(typenamef90 typenamef90.f90 ../util/mtestf90.f90)
  add_executable(typesnamef90 typesnamef90.f90 ../util/mtestf90.f90)
  add_executable(typesubf90 typesubf90.f90 ../util/mtestf90.f90)
  target_link_libraries(allctypesf90  simgrid)
 # target_link_libraries(createf90  simgrid)
  target_link_libraries(gaddressf90  simgrid)
 # target_link_libraries(get_elem_d  simgrid)
 # target_link_libraries(get_elem_u  simgrid)
  target_link_libraries(hindex1f90  simgrid)
  target_link_libraries(hindexed_blockf90  simgrid)
  target_link_libraries(indtype  simgrid)
  target_link_libraries(kinds  simgrid)
  target_link_libraries(packef90  simgrid)
 # target_link_libraries(sizeof  simgrid)
 # target_link_libraries(structf  simgrid)
 # target_link_libraries(trf90  simgrid)
  target_link_libraries(typecntsf90  simgrid)
  target_link_libraries(typem2f90  simgrid)
  target_link_libraries(typename3f90  simgrid)
  target_link_libraries(typenamef90  simgrid)
  target_link_libraries(typesnamef90  simgrid)
  target_link_libraries(typesubf90  simgrid)

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
  ${CMAKE_CURRENT_SOURCE_DIR}/allctypesf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/createf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/gaddressf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/get_elem_d.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/get_elem_u.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/hindex1f90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/hindexed_blockf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/indtype.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/kinds.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/packef90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/sizeof.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/structf.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/trf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/typecntsf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/typem2f90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/typename3f90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/typenamef90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/typesnamef90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/typesubf90.f90
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
