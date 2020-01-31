if(enable_smpi AND enable_smpi_MPICH3_testsuite AND SMPI_FORTRAN)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpif90")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  foreach(test gaddressf90 typesnamef90 structf 
             # allctypesf90 createf90 get_elem_d get_elem_u
	       hindex1f90
	       indtype kinds
	     # packef90 sizeof  trf90 typecntsf90 typem2f90 typename3f90
	     # typenamef90 hindexed_blockf90
	       typesubf90)
    add_executable(${test} EXCLUDE_FROM_ALL ${test}.f90)
    add_dependencies(tests ${test})
    target_link_libraries(${test} simgrid mtest_f90)
    if((CMAKE_Fortran_COMPILER_ID MATCHES "GNU") AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "9.99"))
      set_property(SOURCE ${test}.f90 PROPERTY COMPILE_FLAGS -std=legacy)
    endif()
  endforeach()
endif()

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
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
