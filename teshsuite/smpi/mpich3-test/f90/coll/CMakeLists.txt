if(enable_smpi AND enable_smpi_MPICH3_testsuite AND SMPI_FORTRAN)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpif90")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  foreach(test alltoallvf90
           allredint8f90 allredopttf90 alltoallwf90
           exscanf90 inplacef90
           nonblockingf90 nonblocking_inpf90
           redscatf90 red_scat_blockf90 reducelocalf90
           split_typef90 uallreducef90 vw_inplacef90)
    add_executable(${test} EXCLUDE_FROM_ALL ${test}.f90)
    add_dependencies(tests ${test})
    target_link_libraries(${test} simgrid mtest_f90)
    if((CMAKE_Fortran_COMPILER_ID MATCHES "GNU") AND (NOT CMAKE_Fortran_COMPILER_VERSION VERSION_LESS "10.0"))
      set_property(SOURCE ${test}.f90 PROPERTY COMPILE_FLAGS -std=legacy)
    endif()
  endforeach()
endif()

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
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
