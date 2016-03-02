if(enable_smpi AND enable_smpi_MPICH3_testsuite)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  add_executable(cmfree cmfree.c)
  add_executable(cmsplit2 cmsplit2.c)
  add_executable(cmsplit cmsplit.c)
  add_executable(cmsplit_type cmsplit_type.c)
  add_executable(commcreate1 commcreate1.c)
  add_executable(comm_create_group comm_create_group.c)
  add_executable(comm_group_half comm_group_half.c)
  add_executable(comm_group_rand comm_group_rand.c)
#  add_executable(comm_idup comm_idup.c)
#  add_executable(comm_idup_mul comm_idup_mul.c)
#  add_executable(comm_idup_overlap comm_idup_overlap.c)
  add_executable(comm_info comm_info.c)
#  add_executable(commname commname.c)
  add_executable(ctxalloc ctxalloc.c)
  add_executable(ctxsplit ctxsplit.c)
  add_executable(dup dup.c)
#  add_executable(dupic dupic.c)
  add_executable(dup_with_info dup_with_info.c)
#  add_executable(ic1 ic1.c)
#  add_executable(ic2 ic2.c)
#  add_executable(iccreate iccreate.c)
#  add_executable(icgroup icgroup.c)
#  add_executable(icm icm.c)
#  add_executable(icsplit icsplit.c)
#  add_executable(probe-intercomm probe-intercomm.c)

  target_link_libraries(cmfree simgrid mtest_c)
  target_link_libraries(cmsplit2 simgrid mtest_c)
  target_link_libraries(cmsplit simgrid mtest_c)
  target_link_libraries(cmsplit_type simgrid mtest_c)
  target_link_libraries(commcreate1 simgrid mtest_c)
  target_link_libraries(comm_create_group simgrid mtest_c)
  target_link_libraries(comm_group_half simgrid mtest_c)
  target_link_libraries(comm_group_rand simgrid mtest_c)
#  target_link_libraries(comm_idup simgrid mtest_c)
  target_link_libraries(comm_info simgrid mtest_c)
#  target_link_libraries(commname simgrid mtest_c)
  target_link_libraries(ctxalloc simgrid mtest_c)
  target_link_libraries(ctxsplit simgrid mtest_c)
  target_link_libraries(dup simgrid mtest_c)
#  target_link_libraries(dupic simgrid mtest_c)
  target_link_libraries(dup_with_info simgrid mtest_c)
#  target_link_libraries(ic1 simgrid mtest_c)
#  target_link_libraries(ic2 simgrid mtest_c)
#  target_link_libraries(iccreate simgrid mtest_c)
#  target_link_libraries(icgroup simgrid mtest_c)
#  target_link_libraries(icm simgrid mtest_c)
#  target_link_libraries(icsplit simgrid mtest_c)
#  target_link_libraries(probe-intercomm simgrid mtest_c)

endif()

set(examples_src
  ${examples_src}
 ${CMAKE_CURRENT_SOURCE_DIR}/cmfree.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/cmsplit2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/cmsplit.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/cmsplit_type.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/commcreate1.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/comm_create_group.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/comm_group_half.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/comm_group_rand.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/comm_idup.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/comm_idup_overlap.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/comm_idup_mul.c 
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
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
