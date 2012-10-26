cmake_minimum_required(VERSION 2.6)

if(enable_smpi)
  set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  set(CMAKE_Fortran_LINKER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
  set(MPICH_FLAGS "-DHAVE_STDLIB_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STRING_H=1 -DUSE_STDARG=1 -DHAVE_LONG_DOUBLE=1 -DHAVE_PROTOTYPES=1 -DHAVE_SIGNAL_H=1 -DHAVE_SIGACTION=1 -DHAVE_SLEEP=1 -DHAVE_SYSCONF=1")

  include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")
  
  add_executable(attrerr attrerr.c test.c)
 # add_executable(attrt attrt.c test.c)
 # add_executable(commnames commnames.c)
  add_executable(groupcreate groupcreate.c)
  add_executable(grouptest_mpich grouptest.c)
  add_executable(icdup icdup.c)
  add_executable(ictest ictest.c )
  add_executable(ictest2 ictest2.c)
  add_executable(ictest3 ictest3.c)
  
  if(SMPI_F2C)
  #add_executable(attrtest_f attrtest.f)
  #add_executable(commnames_f commnamesf.f)
  endif(SMPI_F2C)

  target_link_libraries(attrerr m simgrid smpi )
 # target_link_libraries(attrt m simgrid smpi )
 # target_link_libraries(commnames m simgrid smpi )
  target_link_libraries(groupcreate m simgrid smpi )
  target_link_libraries(grouptest_mpich m simgrid smpi )
  target_link_libraries(icdup m simgrid smpi )
  target_link_libraries(ictest m simgrid smpi )
  target_link_libraries(ictest2 m simgrid smpi )
  target_link_libraries(ictest3 m simgrid smpi )

  if(SMPI_F2C)
  #target_link_libraries(attrtest_f m simgrid smpi f2c)
  #target_link_libraries(commnames_f m simgrid smpi f2c)
  endif(SMPI_F2C)
  
  set_target_properties(attrerr PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 # set_target_properties(attrt PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 # set_target_properties(commnames PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(groupcreate PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(grouptest_mpich PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(icdup PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(ictest PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(ictest2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(ictest3 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 
 
   if(SMPI_F2C)
  #set_target_properties(commnames_f PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}" )
  #set_target_properties(attrtest_f PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}"  )
  endif(SMPI_F2C)

endif(enable_smpi)

set(tesh_files
  ${tesh_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/context.tesh
  PARENT_SCOPE
  )
set(xml_files
  ${xml_files}
  PARENT_SCOPE
  )
set(examples_src
  ${examples_src}
  ${CMAKE_CURRENT_SOURCE_DIR}/test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/attrerr.c
 # ${CMAKE_CURRENT_SOURCE_DIR}/attrt.c
 # ${CMAKE_CURRENT_SOURCE_DIR}/commnames.c
  ${CMAKE_CURRENT_SOURCE_DIR}/groupcreate.c
  ${CMAKE_CURRENT_SOURCE_DIR}/grouptest.c
  ${CMAKE_CURRENT_SOURCE_DIR}/icdup.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ictest.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ictest2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ictest3.c
  ${CMAKE_CURRENT_SOURCE_DIR}/attrtest.f
  ${CMAKE_CURRENT_SOURCE_DIR}/commnamesf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/test.h
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/../hostfile
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  PARENT_SCOPE
  )
