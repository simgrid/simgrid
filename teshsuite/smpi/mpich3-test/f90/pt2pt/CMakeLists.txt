cmake_minimum_required(VERSION 2.6)

if(enable_smpi AND SMPI_F2C)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpif90")
  endif()

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
  set(MPICH_FLAGS "-DHAVE_STDLIB_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STRING_H=1 -DUSE_STDARG=1 -DHAVE_LONG_DOUBLE=1 -DHAVE_PROTOTYPES=1 -DHAVE_SIGNAL_H=1 -DHAVE_SIGACTION=1 -DHAVE_SLEEP=1 -DHAVE_SYSCONF=1  -Wno-error=unused-variable")
  include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")

  add_executable(allpairf90 allpairf90.f90 ../util/mtestf90.f90)
  add_executable(greqf90 greqf90.f90 dummyf90.f90 ../util/mtestf90.f90)
#  add_executable(mprobef90 mprobef90.f90 ../util/mtestf90.f90)
  add_executable(statusesf90 statusesf90.f90 ../util/mtestf90.f90)
  target_link_libraries(allpairf90  simgrid)
  target_link_libraries(greqf90  simgrid)
#  target_link_libraries(mprobef90  simgrid)
  target_link_libraries(statusesf90  simgrid)
  set_target_properties(allpairf90 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(greqf90 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
#  set_target_properties(mprobef90 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(statusesf90 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")

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
  ${CMAKE_CURRENT_SOURCE_DIR}/allpairf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/dummyf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/greqf90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/mprobef90.f90
  ${CMAKE_CURRENT_SOURCE_DIR}/statusesf90.f90
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
