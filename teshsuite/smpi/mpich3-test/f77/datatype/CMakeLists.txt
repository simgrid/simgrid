if(enable_smpi AND enable_smpi_MPICH3_testsuite AND SMPI_FORTRAN)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/typeaints.h.in ${CMAKE_CURRENT_SOURCE_DIR}/typeaints.h @ONLY)

#  add_executable(allctypesf allctypesf.f)
  add_executable(gaddressf gaddressf.f)
#  add_executable(hindex1f hindex1f.f)
#  add_executable(hindexed_blockf hindexed_blockf.f)
#  add_executable(packef packef.f)
#  add_executable(typecntsf typecntsf.f)
#  add_executable(typem2f typem2f.f)
#  add_executable(typename3f typename3f.f)
#  add_executable(typenamef typenamef.f)
#  add_executable(typesnamef typesnamef.f)
#  add_executable(typesubf typesubf.f)

#  target_link_libraries(allctypesf simgrid mtest_f77)
  target_link_libraries(gaddressf simgrid mtest_f77)
#  target_link_libraries(hindex1f simgrid mtest_f77)
#  target_link_libraries(hindexed_blockf simgrid mtest_f77)
#  target_link_libraries(packef simgrid mtest_f77)
#  target_link_libraries(typecntsf simgrid mtest_f77)
#  target_link_libraries(typem2f simgrid mtest_f77)
#  target_link_libraries(typename3f simgrid mtest_f77)
#  target_link_libraries(typenamef simgrid mtest_f77)
#  target_link_libraries(typesnamef simgrid mtest_f77)
#  target_link_libraries(typesubf simgrid mtest_f77)
endif()

set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/allctypesf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/gaddressf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/hindex1f.f
  ${CMAKE_CURRENT_SOURCE_DIR}/hindexed_blockf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/packef.f
  ${CMAKE_CURRENT_SOURCE_DIR}/typeaints.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/typecntsf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/typem2f.f
  ${CMAKE_CURRENT_SOURCE_DIR}/typename3f.f
  ${CMAKE_CURRENT_SOURCE_DIR}/typenamef.f
  ${CMAKE_CURRENT_SOURCE_DIR}/typesnamef.f
  ${CMAKE_CURRENT_SOURCE_DIR}/typesubf.f
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
