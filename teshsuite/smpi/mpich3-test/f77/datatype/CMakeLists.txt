if(enable_smpi AND enable_smpi_MPICH3_testsuite AND SMPI_FORTRAN)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories(${CMAKE_CURRENT_BINARY_DIR})

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/typeaints.h.in typeaints.h @ONLY)

  foreach(test gaddressf
             allctypesf hindex1f hindexed_blockf packef typecntsf
             typem2f typename3f typenamef typesnamef
	       typesubf)
    add_executable(${test} EXCLUDE_FROM_ALL ${test}.f)
    add_dependencies(tests ${test})
    target_link_libraries(${test} simgrid mtest_f77)	
  endforeach()
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
