if(enable_smpi AND enable_smpi_MPICH3_testsuite AND SMPI_FORTRAN)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

#  add_executable(allredint8f allredint8f.f)
#  add_executable(allredopttf allredopttf.f)
  add_executable(alltoallvf alltoallvf.f)
#  add_executable(alltoallwf alltoallwf.f)
  add_executable(exscanf exscanf.f)
  add_executable(inplacef inplacef.f)
 # add_executable(nonblockingf nonblockingf.f)
 # add_executable(nonblocking_inpf nonblocking_inpf.f)
  add_executable(red_scat_blockf red_scat_blockf.f)
  add_executable(redscatf redscatf.f)
  add_executable(reducelocalf reducelocalf.f)
  add_executable(split_typef split_typef.f)
  add_executable(uallreducef uallreducef.f)
  add_executable(vw_inplacef vw_inplacef.f)

#  target_link_libraries(allredint8f simgrid mtest_f77)
#  target_link_libraries(allredopttf simgrid mtest_f77)
  target_link_libraries(alltoallvf simgrid mtest_f77)
#  target_link_libraries(alltoallwf simgrid mtest_f77)
  target_link_libraries(exscanf simgrid mtest_f77)
  target_link_libraries(inplacef simgrid mtest_f77)
 # target_link_libraries(nonblockingf simgrid mtest_f77)
 # target_link_libraries(nonblocking_inpf simgrid mtest_f77)
  target_link_libraries(red_scat_blockf simgrid mtest_f77)
  target_link_libraries(redscatf simgrid mtest_f77)
  target_link_libraries(reducelocalf simgrid mtest_f77)
  target_link_libraries(split_typef simgrid mtest_f77)
  target_link_libraries(uallreducef simgrid mtest_f77)
  target_link_libraries(vw_inplacef simgrid mtest_f77)
endif()

set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/allredint8f.f
  ${CMAKE_CURRENT_SOURCE_DIR}/allredopttf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/alltoallvf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/alltoallwf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/exscanf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/inplacef.f
  ${CMAKE_CURRENT_SOURCE_DIR}/nonblockingf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/nonblocking_inpf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/red_scat_blockf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/redscatf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/reducelocalf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/split_typef.f
  ${CMAKE_CURRENT_SOURCE_DIR}/uallreducef.f
  ${CMAKE_CURRENT_SOURCE_DIR}/vw_inplacef.f
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
