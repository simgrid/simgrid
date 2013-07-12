cmake_minimum_required(VERSION 2.6)

if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
  set(MPICH_FLAGS "-DHAVE_STDLIB_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STRING_H=1 -DUSE_STDARG=1 -DHAVE_LONG_DOUBLE=1 -DHAVE_PROTOTYPES=1 -DHAVE_SIGNAL_H=1 -DHAVE_SIGACTION=1 -DHAVE_SLEEP=1 -DHAVE_SYSCONF=1")
  include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_BINARY_DIR}/../include/")


  add_executable(cmake_install cmake_install.cmake ../util/mtest.c)
  add_executable(cmfree cmfree.c ../util/mtest.c)
  add_executable(cmsplit2 cmsplit2.c ../util/mtest.c)
  add_executable(cmsplit cmsplit.c ../util/mtest.c)
  add_executable(cmsplit_type cmsplit_type.c ../util/mtest.c)
  add_executable(commcreate1 commcreate1.c ../util/mtest.c)
  add_executable(comm_create_group comm_create_group.c ../util/mtest.c)
  add_executable(comm_group_half comm_group_half.c ../util/mtest.c)
  add_executable(comm_group_rand comm_group_rand.c ../util/mtest.c)
 # add_executable(comm_idup comm_idup.c ../util/mtest.c)
  add_executable(comm_info comm_info.c ../util/mtest.c)
  add_executable(commname commname.c ../util/mtest.c)
  add_executable(ctxalloc ctxalloc.c ../util/mtest.c)
  add_executable(ctxsplit ctxsplit.c ../util/mtest.c)
  add_executable(dup dup.c ../util/mtest.c)
  add_executable(dupic dupic.c ../util/mtest.c)
  add_executable(dup_with_info dup_with_info.c ../util/mtest.c)
  add_executable(ic1 ic1.c ../util/mtest.c)
  add_executable(ic2 ic2.c ../util/mtest.c)
  add_executable(iccreate iccreate.c ../util/mtest.c)
  add_executable(icgroup icgroup.c ../util/mtest.c)
  add_executable(icm icm.c ../util/mtest.c)
  add_executable(icsplit icsplit.c ../util/mtest.c)
  add_executable(probe-intercomm probe-intercomm.c ../util/mtest.c)



  target_link_libraries(cmake_install  simgrid)
  target_link_libraries(cmfree  simgrid)
  target_link_libraries(cmsplit2  simgrid)
  target_link_libraries(cmsplit  simgrid)
  target_link_libraries(cmsplit_type  simgrid)
  target_link_libraries(commcreate1  simgrid)
  target_link_libraries(comm_create_group  simgrid)
  target_link_libraries(comm_group_half  simgrid)
  target_link_libraries(comm_group_rand  simgrid)
 # target_link_libraries(comm_idup  simgrid)
  target_link_libraries(comm_info  simgrid)
  target_link_libraries(commname  simgrid)
  target_link_libraries(ctxalloc  simgrid)
  target_link_libraries(ctxsplit  simgrid)
  target_link_libraries(dup  simgrid)
  target_link_libraries(dupic  simgrid)
  target_link_libraries(dup_with_info  simgrid)
  target_link_libraries(ic1  simgrid)
  target_link_libraries(ic2  simgrid)
  target_link_libraries(iccreate  simgrid)
  target_link_libraries(icgroup  simgrid)
  target_link_libraries(icm  simgrid)
  target_link_libraries(icsplit  simgrid)
  target_link_libraries(probe-intercomm  simgrid)



 set_target_properties(cmake_install PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(cmfree PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(cmsplit2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(cmsplit PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(cmsplit_type PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(commcreate1 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(comm_create_group PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(comm_group_half PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(comm_group_rand PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(comm_idup PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(comm_info PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(commname PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(ctxalloc PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(ctxsplit PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(dup PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(dupic PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(dup_with_info PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(ic1 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(ic2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(iccreate PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(icgroup PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(icm PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(icsplit PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(probe-intercomm PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")

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
 ${CMAKE_CURRENT_SOURCE_DIR}/cmake_install.cmake 
 ${CMAKE_CURRENT_SOURCE_DIR}/cmfree.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/cmsplit2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/cmsplit.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/cmsplit_type.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/commcreate1.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/comm_create_group.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/comm_group_half.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/comm_group_rand.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/comm_idup.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/comm_info.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/commname.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/ctxalloc.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/ctxsplit.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/dup.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/dupic.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/dup_with_info.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/ic1.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/ic2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/iccreate.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/icgroup.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/icm.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/icsplit.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/probe-intercomm.c 
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/runtests
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE
  )
