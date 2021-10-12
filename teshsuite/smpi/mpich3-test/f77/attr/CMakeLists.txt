if(enable_smpi AND enable_smpi_MPICH3_testsuite AND SMPI_FORTRAN)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")

  foreach(test attrmpi1f
               baseattrf baseattr2f
               commattrf commattr2f commattr3f
	       typeattrf typeattr2f typeattr3f)
    add_executable(${test} EXCLUDE_FROM_ALL ${test}.f)
    add_dependencies(tests ${test})
    target_link_libraries(${test} simgrid mtest_f77)
  endforeach()
endif()

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
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE
)
