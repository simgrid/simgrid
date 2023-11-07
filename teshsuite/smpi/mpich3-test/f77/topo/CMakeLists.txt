if(enable_smpi AND enable_testsuite_smpi_MPICH3 AND SMPI_FORTRAN)
  set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  foreach(test cartcrf
             # dgraph_unwgtf dgraph_wgtf
	       )
    add_executable(${test} EXCLUDE_FROM_ALL ${test}.f)
    add_dependencies(tests ${test})
    target_link_libraries(${test} simgrid mtest_f77)
  endforeach()
endif()

set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/dgraph_wgtf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/dgraph_unwgtf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/cartcrf.f
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
