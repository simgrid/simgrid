if(enable_smpi AND enable_smpi_MPICH3_testsuite)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  add_executable(anyall anyall.c)
  add_executable(bottom bottom.c)
#  add_executable(big_count_status big_count_status.c)
#  add_executable(bsend1 bsend1.c)
#  add_executable(bsend2 bsend2.c)
#  add_executable(bsend3 bsend3.c)
#  add_executable(bsend4 bsend4.c)
#  add_executable(bsend5 bsend5.c)
#  add_executable(bsendalign bsendalign.c)
#  add_executable(bsendfrag bsendfrag.c)
#  add_executable(bsendpending bsendpending.c)
#  add_executable(cancelrecv cancelrecv.c)
  add_executable(eagerdt eagerdt.c)
#  add_executable(greq1 greq1.c)
#  add_executable(icsend icsend.c)
  add_executable(inactivereq inactivereq.c)
  add_executable(isendself isendself.c)
  add_executable(isendselfprobe isendselfprobe.c)
  add_executable(issendselfcancel issendselfcancel.c)
#  add_executable(large_message large_message.c)
  add_executable(mprobe mprobe.c)
  add_executable(pingping pingping.c)
  add_executable(probenull probenull.c)
  add_executable(probe-unexp probe-unexp.c)
#  add_executable(pscancel pscancel.c)
#  add_executable(rcancel rcancel.c)
#  add_executable(rqfreeb rqfreeb.c)
#  add_executable(rqstatus rqstatus.c)
#  add_executable(scancel2 scancel2.c)
#  add_executable(scancel scancel.c)
  add_executable(sendall sendall.c)
  add_executable(sendflood sendflood.c)
  add_executable(sendrecv1 sendrecv1.c)
  add_executable(sendrecv2 sendrecv2.c)
  add_executable(sendrecv3 sendrecv3.c)
#  add_executable(sendself sendself.c)
  add_executable(waitany-null waitany-null.c)
  add_executable(waittestnull waittestnull.c)

  target_link_libraries(anyall simgrid mtest_c)
#  target_link_libraries(big_count_status simgrid mtest_c)
  target_link_libraries(bottom simgrid mtest_c)
#  target_link_libraries(bsend1 simgrid mtest_c)
#  target_link_libraries(bsend2 simgrid mtest_c)
#  target_link_libraries(bsend3 simgrid mtest_c)
#  target_link_libraries(bsend4 simgrid mtest_c)
#  target_link_libraries(bsend5 simgrid mtest_c)
#  target_link_libraries(bsendalign simgrid mtest_c)
#  target_link_libraries(bsendfrag simgrid mtest_c)
#  target_link_libraries(bsendpending simgrid mtest_c)
#  target_link_libraries(cancelrecv simgrid mtest_c)
  target_link_libraries(eagerdt simgrid mtest_c)
#  target_link_libraries(greq1 simgrid mtest_c)
#  target_link_libraries(icsend simgrid mtest_c)
  target_link_libraries(inactivereq simgrid mtest_c)
  target_link_libraries(isendself simgrid mtest_c)
  target_link_libraries(isendselfprobe simgrid mtest_c)
  target_link_libraries(issendselfcancel simgrid mtest_c)
#  target_link_libraries(large_message simgrid mtest_c)
  target_link_libraries(mprobe simgrid mtest_c)
  target_link_libraries(pingping simgrid mtest_c)
  target_link_libraries(probenull simgrid mtest_c)
  target_link_libraries(probe-unexp simgrid mtest_c)
#  target_link_libraries(pscancel simgrid mtest_c)
#  target_link_libraries(rcancel simgrid mtest_c)
#  target_link_libraries(rqfreeb simgrid mtest_c)
#  target_link_libraries(rqstatus simgrid mtest_c)
#  target_link_libraries(scancel2 simgrid mtest_c)
#  target_link_libraries(scancel simgrid mtest_c)
  target_link_libraries(sendall simgrid mtest_c)
  target_link_libraries(sendflood simgrid mtest_c)
  target_link_libraries(sendrecv1 simgrid mtest_c)
  target_link_libraries(sendrecv2 simgrid mtest_c)
  target_link_libraries(sendrecv3 simgrid mtest_c)
#  target_link_libraries(sendself simgrid mtest_c)
  target_link_libraries(waitany-null simgrid mtest_c)
  target_link_libraries(waittestnull simgrid mtest_c)
endif()

set(examples_src
  ${examples_src}
 ${CMAKE_CURRENT_SOURCE_DIR}/anyall.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/big_count_status.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/bottom.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/bsend1.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/bsend2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/bsend3.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/bsend4.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/bsend5.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/bsendalign.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/bsendfrag.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/bsendpending.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/cancelrecv.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/eagerdt.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/greq1.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/icsend.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/inactivereq.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/isendself.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/isendselfprobe.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/issendselfcancel.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/large_message.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/mprobe.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/pingping.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/probenull.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/probe-unexp.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/pscancel.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/rcancel.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/rqfreeb.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/rqstatus.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/scancel2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/scancel.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/sendall.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/sendflood.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv1.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv3.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/sendself.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/waitany-null.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/waittestnull.c 
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  PARENT_SCOPE)
