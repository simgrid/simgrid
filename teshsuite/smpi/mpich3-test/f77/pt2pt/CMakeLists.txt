cmake_minimum_required(VERSION 2.6)

if(enable_smpi AND SMPI_F2C)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  set(CMAKE_INCLUDE_CURRENT_DIR ON)
  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
  set(MPICH_FLAGS "-DHAVE_STDLIB_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STRING_H=1 -DUSE_STDARG=1 -DHAVE_LONG_DOUBLE=1 -DHAVE_PROTOTYPES=1 -DHAVE_SIGNAL_H=1 -DHAVE_SIGACTION=1 -DHAVE_SLEEP=1 -DHAVE_SYSCONF=1  -Wno-error=unused-variable -Wno-implicit -Wno-implicit-function-declaration")
  include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")


  add_executable(allpairf allpairf.f ../util/mtestf.f)
  add_executable(greqf greqf.f dummyf.f ../util/mtestf.f)
  #add_executable(mprobef mprobef.f ../util/mtestf.f)
  add_executable(statusesf statusesf.f ../util/mtestf.f)

  target_link_libraries(allpairf  simgrid)
  target_link_libraries(greqf  simgrid)
  #target_link_libraries(mprobef  simgrid)
  target_link_libraries(statusesf  simgrid)

 set_target_properties(allpairf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(greqf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 #set_target_properties(mprobef PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(statusesf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")

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
  ${CMAKE_CURRENT_SOURCE_DIR}/allpairf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/attr1aints.h
  ${CMAKE_CURRENT_SOURCE_DIR}/dummyf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/greqf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/mprobef.f
  ${CMAKE_CURRENT_SOURCE_DIR}/statusesf.f
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
