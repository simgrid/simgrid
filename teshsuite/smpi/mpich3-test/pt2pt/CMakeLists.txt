cmake_minimum_required(VERSION 2.6)

if(enable_smpi)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
  set(MPICH_FLAGS "-DHAVE_STDLIB_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STRING_H=1 -DUSE_STDARG=1 -DHAVE_LONG_DOUBLE=1 -DHAVE_PROTOTYPES=1 -DHAVE_SIGNAL_H=1 -DHAVE_SIGACTION=1 -DHAVE_SLEEP=1 -DHAVE_SYSCONF=1  -Wno-error=unused-variable")
  include_directories("${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_BINARY_DIR}/../include/")


  add_executable(anyall anyall.c ../util/mtest.c)
  add_executable(bottom bottom.c ../util/mtest.c)
  add_executable(bsend1 bsend1.c ../util/mtest.c)
  add_executable(bsend2 bsend2.c ../util/mtest.c)
  add_executable(bsend3 bsend3.c ../util/mtest.c)
  add_executable(bsend4 bsend4.c ../util/mtest.c)
  add_executable(bsend5 bsend5.c ../util/mtest.c)
  add_executable(bsendalign bsendalign.c ../util/mtest.c)
  add_executable(bsendfrag bsendfrag.c ../util/mtest.c)
  add_executable(bsendpending bsendpending.c ../util/mtest.c)
  add_executable(cancelrecv cancelrecv.c ../util/mtest.c)
  add_executable(eagerdt eagerdt.c ../util/mtest.c)
  add_executable(greq1 greq1.c ../util/mtest.c)
  add_executable(icsend icsend.c ../util/mtest.c)
  add_executable(inactivereq inactivereq.c ../util/mtest.c)
  add_executable(isendself isendself.c ../util/mtest.c)
  add_executable(isendselfprobe isendselfprobe.c ../util/mtest.c)
  add_executable(large_message large_message.c ../util/mtest.c)
  add_executable(mprobe mprobe.c ../util/mtest.c)
  add_executable(pingping pingping.c ../util/mtest.c)
  add_executable(probenull probenull.c ../util/mtest.c)
  add_executable(probe-unexp probe-unexp.c ../util/mtest.c)
  add_executable(pscancel pscancel.c ../util/mtest.c)
  add_executable(rcancel rcancel.c ../util/mtest.c)
  add_executable(rqfreeb rqfreeb.c ../util/mtest.c)
  add_executable(rqstatus rqstatus.c ../util/mtest.c)
  add_executable(scancel2 scancel2.c ../util/mtest.c)
  add_executable(scancel scancel.c ../util/mtest.c)
  add_executable(sendall sendall.c ../util/mtest.c)
  add_executable(sendflood sendflood.c ../util/mtest.c)
  add_executable(sendrecv1 sendrecv1.c ../util/mtest.c)
  add_executable(sendrecv2 sendrecv2.c ../util/mtest.c)
  add_executable(sendrecv3 sendrecv3.c ../util/mtest.c)
  add_executable(sendself sendself.c ../util/mtest.c)
  add_executable(waitany-null waitany-null.c ../util/mtest.c)
  add_executable(waittestnull waittestnull.c ../util/mtest.c)



  target_link_libraries(anyall  simgrid)
  target_link_libraries(bottom  simgrid)
  target_link_libraries(bsend1  simgrid)
  target_link_libraries(bsend2  simgrid)
  target_link_libraries(bsend3  simgrid)
  target_link_libraries(bsend4  simgrid)
  target_link_libraries(bsend5  simgrid)
  target_link_libraries(bsendalign  simgrid)
  target_link_libraries(bsendfrag  simgrid)
  target_link_libraries(bsendpending  simgrid)
  target_link_libraries(cancelrecv  simgrid)
  target_link_libraries(eagerdt  simgrid)
  target_link_libraries(greq1  simgrid)
  target_link_libraries(icsend  simgrid)
  target_link_libraries(inactivereq  simgrid)
  target_link_libraries(isendself  simgrid)
  target_link_libraries(isendselfprobe  simgrid)
  target_link_libraries(large_message  simgrid)
  target_link_libraries(mprobe  simgrid)
  target_link_libraries(pingping  simgrid)
  target_link_libraries(probenull  simgrid)
  target_link_libraries(probe-unexp  simgrid)
  target_link_libraries(pscancel  simgrid)
  target_link_libraries(rcancel  simgrid)
  target_link_libraries(rqfreeb  simgrid)
  target_link_libraries(rqstatus  simgrid)
  target_link_libraries(scancel2  simgrid)
  target_link_libraries(scancel  simgrid)
  target_link_libraries(sendall  simgrid)
  target_link_libraries(sendflood  simgrid)
  target_link_libraries(sendrecv1  simgrid)
  target_link_libraries(sendrecv2  simgrid)
  target_link_libraries(sendrecv3  simgrid)
  target_link_libraries(sendself  simgrid)
  target_link_libraries(waitany-null  simgrid)
  target_link_libraries(waittestnull  simgrid)



 set_target_properties(anyall PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(bottom PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(bsend1 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(bsend2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(bsend3 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(bsend4 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(bsend5 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(bsendalign PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(bsendfrag PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(bsendpending PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(cancelrecv PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(eagerdt PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(greq1 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(icsend PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(inactivereq PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(isendself PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(isendselfprobe PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(large_message PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(mprobe PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(pingping PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(probenull PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(probe-unexp PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(pscancel PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(rcancel PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(rqfreeb PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(rqstatus PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(scancel2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(scancel PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(sendall PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(sendflood PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(sendrecv1 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(sendrecv2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(sendrecv3 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(sendself PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(waitany-null PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(waittestnull PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")

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
 ${CMAKE_CURRENT_SOURCE_DIR}/anyall.c 
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
