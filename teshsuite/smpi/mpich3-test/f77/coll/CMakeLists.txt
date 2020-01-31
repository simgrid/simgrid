if(enable_smpi AND enable_smpi_MPICH3_testsuite AND SMPI_FORTRAN)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  foreach(test alltoallvf
           allredint8f allredopttf alltoallwf
           exscanf inplacef
           nonblockingf nonblocking_inpf
           redscatf red_scat_blockf reducelocalf
           split_typef uallreducef vw_inplacef)
    add_executable(${test} EXCLUDE_FROM_ALL ${test}.f)
    add_dependencies(tests ${test})
    target_link_libraries(${test} simgrid mtest_f77)
    if((CMAKE_Fortran_COMPILER_ID MATCHES "GNU") AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "9.99"))
      set_property(SOURCE ${test}.f PROPERTY COMPILE_FLAGS -std=legacy)
    endif()
  endforeach()
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
