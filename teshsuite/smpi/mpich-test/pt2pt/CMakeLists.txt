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
  add_executable(overtake overtake.c test.c)
  add_executable(bsendtest bsendtest.c test.c)
  add_executable(relrank relrank.c test.c)
  add_executable(sendrecv_mpich sendrecv.c test.c)
  add_executable(persistent persistent.c)
  add_executable(flood flood.c test.c)
  add_executable(flood2 flood2.c test.c)
  add_executable(persist persist.c)
  add_executable(persist2 persist2.c)
  add_executable(sendrecv2 sendrecv2.c dtypes.c gcomm.c)
  add_executable(sendrecv3 sendrecv3.c dtypes.c gcomm.c)
  add_executable(sendrecv4 sendrecv4.c dtypes.c gcomm.c)
  add_executable(irsend irsend.c dtypes.c gcomm.c)
  add_executable(irsendinit irsendinit.c dtypes.c gcomm.c)
  add_executable(longmsgs longmsgs.c)
  add_executable(testsome testsome.c dtypes.c gcomm.c)
  add_executable(testall_mpich testall.c dtypes.c gcomm.c)
  add_executable(sndrcv sndrcv.c dtypes.c gcomm.c)
  add_executable(sndrcvrep sndrcvrep.c)
  add_executable(sndrcvrpl2 sndrcvrpl2.c)
  add_executable(getelm getelm.c )
  add_executable(self self.c test.c)
  add_executable(typelb typelb.c test.c)
  add_executable(structlb structlb.c test.c)
  add_executable(typeub typeub.c test.c)
  add_executable(typeub2 typeub2.c)
  add_executable(typeub3 typeub3.c)
  add_executable(typecreate typecreate.c)
  add_executable(reqcreate reqcreate.c)
  add_executable(reqfree reqfree.c)
  add_executable(isndrcv isndrcv.c test.c)
  add_executable(nullproc nullproc.c)
  add_executable(nullproc2 nullproc2.c)
  add_executable(trunc trunc.c)
  add_executable(truncmult truncmult.c)
  add_executable(typetest typetest.c test.c)
  add_executable(typebase typebase.c)
  add_executable(ssendtest ssendtest.c test.c)
  add_executable(ssendtest2 ssendtest2.c)
  add_executable(issendtest issendtest.c test.c)
  add_executable(issend2 issend2.c)
  add_executable(issendinit issendinit.c test.c)
  add_executable(testtypes testtypes.c test.c)
  add_executable(dataalign dataalign.c)
  add_executable(dtyperecv dtyperecv.c)
  add_executable(dtypelife dtypelife.c test.c)
  add_executable(htmsg htmsg.c)
  add_executable(isendtest isendtest.c)
  add_executable(third third.c test.c)
  add_executable(fourth fourth.c test.c)
  add_executable(fifth fifth.c test.c)
  add_executable(sixth sixth.c test.c)
  add_executable(probe probe.c test.c)
  add_executable(nblock nblock.c )
  add_executable(sendmany sendmany.c) 
  add_executable(order order.c)
  add_executable(pack pack.c)
  add_executable(probe1 probe1.c test.c)
  add_executable(testtest1 testtest1.c test.c) 
  add_executable(hvectest hvectest.c test.c)
  add_executable(hvectest2 hvectest2.c test.c)
  add_executable(hvec hvec.c)
  add_executable(hindexed hindexed.c) 
  add_executable(irecvtest irecvtest.c test.c)
  add_executable(nbtest nbtest.c)
  add_executable(waitany waitany.c)
  add_executable(waitall waitall.c)
  add_executable(waitall2 waitall2.c)
  add_executable(waitall3 waitall3.c)
  add_executable(waitall4 waitall4.c)
  add_executable(commit commit.c)
  add_executable(cancel cancel.c)
  add_executable(cancel2 cancel2.c)
  add_executable(cancel3 cancel3.c)
  add_executable(cancelmessages cancelmessages.c)
  add_executable(cancelibm cancelibm.c)
  add_executable(cancelissend cancelissend.c)
  add_executable(sendorder sendorder.c)
  add_executable(exittest exittest.c)
  add_executable(selfvsworld selfvsworld.c)
  if(SMPI_F2C)
    add_executable(secondf secondf.f)
#    add_executable(allpair2 allpair2.f)
#    add_executable(allpair allpair.f)
    add_executable(isendf isendf.f)
    add_executable(pingpong_f pingpong.f)
    add_executable(send1 send1.f)
    add_executable(sendcplx sendcplx.f)
    add_executable(sendfort sendfort.f)
#    add_executable(structf structf.f)
    add_executable(typebasef typebasef.f)
  endif()

  target_link_libraries(overtake simgrid smpi )
  target_link_libraries(bsendtest simgrid smpi )
  target_link_libraries(relrank simgrid smpi )
  target_link_libraries(sendrecv_mpich simgrid smpi )
  target_link_libraries(persistent simgrid smpi )
  target_link_libraries(flood simgrid smpi )
  target_link_libraries(flood2 simgrid smpi )
  target_link_libraries(persist simgrid smpi )
  target_link_libraries(persist2 simgrid smpi )
  target_link_libraries(sendrecv2 simgrid smpi )
  target_link_libraries(sendrecv3 simgrid smpi )
  target_link_libraries(sendrecv4 simgrid smpi )
  target_link_libraries(irsend simgrid smpi )
  target_link_libraries(irsendinit simgrid smpi )
  target_link_libraries(longmsgs simgrid smpi )
  target_link_libraries(testsome simgrid smpi )
  target_link_libraries(testall_mpich simgrid smpi )
  target_link_libraries(sndrcv simgrid smpi )
  target_link_libraries(sndrcvrep simgrid smpi )
  target_link_libraries(sndrcvrpl2 simgrid smpi )
  target_link_libraries(getelm simgrid smpi )
  target_link_libraries(self simgrid smpi )
  target_link_libraries(typelb simgrid smpi )
  target_link_libraries(structlb simgrid smpi )
  target_link_libraries(typeub simgrid smpi )
  target_link_libraries(typeub2 simgrid smpi )
  target_link_libraries(typeub3 simgrid smpi )
  target_link_libraries(typecreate simgrid smpi )
  target_link_libraries(reqcreate simgrid smpi )
  target_link_libraries(reqfree simgrid smpi )
  target_link_libraries(isndrcv simgrid smpi )
  target_link_libraries(nullproc simgrid smpi )
  target_link_libraries(nullproc2 simgrid smpi )
  target_link_libraries(trunc simgrid smpi )
  target_link_libraries(truncmult simgrid smpi )
  target_link_libraries(typetest simgrid smpi )
  target_link_libraries(typebase simgrid smpi )
  target_link_libraries(ssendtest simgrid smpi )
  target_link_libraries(ssendtest2 simgrid smpi )
  target_link_libraries(issendtest simgrid smpi )
  target_link_libraries(issend2 simgrid smpi )
  target_link_libraries(issendinit simgrid smpi )
  target_link_libraries(testtypes simgrid smpi )
  target_link_libraries(dataalign simgrid smpi )
  target_link_libraries(dtyperecv simgrid smpi )
  target_link_libraries(dtypelife simgrid smpi )
  target_link_libraries(htmsg simgrid smpi )
  target_link_libraries(isendtest simgrid smpi )
  target_link_libraries(third simgrid smpi )
  target_link_libraries(fourth simgrid smpi )
  target_link_libraries(fifth simgrid smpi )
  target_link_libraries(sixth simgrid smpi )
  target_link_libraries(probe simgrid smpi )
  target_link_libraries(nblock simgrid smpi )
  target_link_libraries(sendmany simgrid smpi )
  target_link_libraries(order simgrid smpi )
  target_link_libraries(pack simgrid smpi )
  target_link_libraries(probe1 simgrid smpi )
  target_link_libraries(testtest1  simgrid smpi )
  target_link_libraries(hvectest simgrid smpi )
  target_link_libraries(hvectest2 simgrid smpi )
  target_link_libraries(hvec simgrid smpi )
  target_link_libraries(hindexed simgrid smpi )
  target_link_libraries(irecvtest simgrid smpi )
  target_link_libraries(nbtest simgrid smpi )
  target_link_libraries(waitany simgrid smpi )
  target_link_libraries(waitall simgrid smpi )
  target_link_libraries(waitall2 simgrid smpi )
  target_link_libraries(waitall3 simgrid smpi )
  target_link_libraries(waitall4 simgrid smpi )
  target_link_libraries(commit simgrid smpi )
  target_link_libraries(cancel simgrid smpi )
  target_link_libraries(cancel2 simgrid smpi )
  target_link_libraries(cancel3 simgrid smpi )
  target_link_libraries(cancelmessages simgrid smpi )
  target_link_libraries(cancelibm simgrid smpi )
  target_link_libraries(cancelissend simgrid smpi )
  target_link_libraries(sendorder simgrid smpi )
  target_link_libraries(exittest simgrid smpi )
  target_link_libraries(selfvsworld simgrid smpi )
  if(SMPI_F2C)
    target_link_libraries(secondf simgrid smpi f2c)
#    target_link_libraries(allpair2 simgrid smpi f2c)
#    target_link_libraries(allpair simgrid smpi f2c)
    target_link_libraries(isendf simgrid smpi f2c)
    target_link_libraries(pingpong_f simgrid smpi f2c)
    target_link_libraries(send1 simgrid smpi f2c)
    target_link_libraries(sendcplx simgrid smpi f2c)
    target_link_libraries(sendfort simgrid smpi f2c)
#    target_link_libraries(structf simgrid smpi f2c)
    target_link_libraries(typebasef simgrid smpi f2c)
  endif()

  set_target_properties(overtake PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(bsendtest PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(relrank PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(sendrecv_mpich PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(persistent PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(flood PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(flood2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(persist PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(persist2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(sendrecv2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(sendrecv3 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(sendrecv4 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(irsend PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(irsendinit PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(longmsgs PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(testsome PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(testall_mpich PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(sndrcv PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(sndrcvrep PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(sndrcvrpl2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(getelm PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(self PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(typelb PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(structlb PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(typeub PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(typeub2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(typeub3 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(typecreate PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(reqcreate PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(reqfree PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(isndrcv PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(nullproc PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(nullproc2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(trunc PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(truncmult PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(typetest PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(typebase PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(ssendtest PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(ssendtest2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(issendtest PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(issend2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(issendinit PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(testtypes PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(dataalign PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(dtyperecv PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(dtypelife PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(htmsg PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(isendtest PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(third PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(fourth PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(fifth PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(sixth PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(probe PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(nblock PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(sendmany PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(order PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(pack PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(probe1 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(testtest1  PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(hvectest PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(hvectest2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(hvec PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(hindexed PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(irecvtest PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(nbtest PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(waitany PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(waitall PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(waitall2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(waitall3 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(waitall4 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(commit PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(cancel PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(cancel2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(cancel3 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(cancelmessages PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(cancelibm PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(cancelissend PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(sendorder PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(exittest PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  set_target_properties(selfvsworld PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  if(SMPI_F2C)
    set_target_properties(secondf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
#    set_target_properties(allpair2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
#    set_target_properties(allpair PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
    set_target_properties(isendf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
    set_target_properties(pingpong_f PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
    set_target_properties(send1 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
    set_target_properties(sendcplx PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
    set_target_properties(sendfort PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
#    set_target_properties(structf PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
    set_target_properties(typebasef PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  endif()
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
  ${CMAKE_CURRENT_SOURCE_DIR}/gcomm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/gcomm.h
  ${CMAKE_CURRENT_SOURCE_DIR}/dtypes.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dtypes.h
  ${CMAKE_CURRENT_SOURCE_DIR}/overtake.c
  ${CMAKE_CURRENT_SOURCE_DIR}/bsendtest.c
  ${CMAKE_CURRENT_SOURCE_DIR}/relrank.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv.c
  ${CMAKE_CURRENT_SOURCE_DIR}/persistent.c
  ${CMAKE_CURRENT_SOURCE_DIR}/flood.c
  ${CMAKE_CURRENT_SOURCE_DIR}/flood2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/persist.c
  ${CMAKE_CURRENT_SOURCE_DIR}/persist2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv3.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sendrecv4.c
  ${CMAKE_CURRENT_SOURCE_DIR}/irsend.c
  ${CMAKE_CURRENT_SOURCE_DIR}/irsendinit.c
  ${CMAKE_CURRENT_SOURCE_DIR}/longmsgs.c
  ${CMAKE_CURRENT_SOURCE_DIR}/testsome.c
  ${CMAKE_CURRENT_SOURCE_DIR}/testall.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sndrcv.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sndrcvrep.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sndrcvrpl2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/getelm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/self.c
  ${CMAKE_CURRENT_SOURCE_DIR}/typelb.c
  ${CMAKE_CURRENT_SOURCE_DIR}/structlb.c
  ${CMAKE_CURRENT_SOURCE_DIR}/typeub.c
  ${CMAKE_CURRENT_SOURCE_DIR}/typeub2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/typeub3.c
  ${CMAKE_CURRENT_SOURCE_DIR}/typecreate.c
  ${CMAKE_CURRENT_SOURCE_DIR}/reqcreate.c
  ${CMAKE_CURRENT_SOURCE_DIR}/reqfree.c
  ${CMAKE_CURRENT_SOURCE_DIR}/isndrcv.c
  ${CMAKE_CURRENT_SOURCE_DIR}/nullproc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/nullproc2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/trunc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/truncmult.c
  ${CMAKE_CURRENT_SOURCE_DIR}/typetest.c
  ${CMAKE_CURRENT_SOURCE_DIR}/typebase.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ssendtest.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ssendtest2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/issendtest.c
  ${CMAKE_CURRENT_SOURCE_DIR}/issend2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/issendinit.c
  ${CMAKE_CURRENT_SOURCE_DIR}/testtypes.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dataalign.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dtyperecv.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dtypelife.c
  ${CMAKE_CURRENT_SOURCE_DIR}/htmsg.c
  ${CMAKE_CURRENT_SOURCE_DIR}/isendtest.c
  ${CMAKE_CURRENT_SOURCE_DIR}/third.c
  ${CMAKE_CURRENT_SOURCE_DIR}/fourth.c
  ${CMAKE_CURRENT_SOURCE_DIR}/fifth.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sixth.c
  ${CMAKE_CURRENT_SOURCE_DIR}/probe.c
  ${CMAKE_CURRENT_SOURCE_DIR}/nblock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sendmany.c
  ${CMAKE_CURRENT_SOURCE_DIR}/order.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pack.c
  ${CMAKE_CURRENT_SOURCE_DIR}/probe1.c
  ${CMAKE_CURRENT_SOURCE_DIR}/testtest1.c
  ${CMAKE_CURRENT_SOURCE_DIR}/hvectest.c
  ${CMAKE_CURRENT_SOURCE_DIR}/hvectest2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/hvec.c
  ${CMAKE_CURRENT_SOURCE_DIR}/hindexed.c
  ${CMAKE_CURRENT_SOURCE_DIR}/irecvtest.c
  ${CMAKE_CURRENT_SOURCE_DIR}/nbtest.c
  ${CMAKE_CURRENT_SOURCE_DIR}/waitany.c
  ${CMAKE_CURRENT_SOURCE_DIR}/waitall.c
  ${CMAKE_CURRENT_SOURCE_DIR}/waitall2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/waitall3.c
  ${CMAKE_CURRENT_SOURCE_DIR}/waitall4.c
  ${CMAKE_CURRENT_SOURCE_DIR}/commit.c
  ${CMAKE_CURRENT_SOURCE_DIR}/cancel.c
  ${CMAKE_CURRENT_SOURCE_DIR}/cancel2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/cancel3.c
  ${CMAKE_CURRENT_SOURCE_DIR}/cancelmessages.c
  ${CMAKE_CURRENT_SOURCE_DIR}/cancelibm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/cancelissend.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sendorder.c
  ${CMAKE_CURRENT_SOURCE_DIR}/exittest.c
  ${CMAKE_CURRENT_SOURCE_DIR}/selfvsworld.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/test.h
  ${CMAKE_CURRENT_SOURCE_DIR}/secondf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/allpair2.f
  ${CMAKE_CURRENT_SOURCE_DIR}/allpair.f
  ${CMAKE_CURRENT_SOURCE_DIR}/isendf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/pingpong.f
  ${CMAKE_CURRENT_SOURCE_DIR}/send1.f
  ${CMAKE_CURRENT_SOURCE_DIR}/sendcplx.f
  ${CMAKE_CURRENT_SOURCE_DIR}/sendfort.f
  ${CMAKE_CURRENT_SOURCE_DIR}/structf.f
  ${CMAKE_CURRENT_SOURCE_DIR}/typebasef.f
  PARENT_SCOPE
  )
set(bin_files
  ${bin_files}
  PARENT_SCOPE
  )
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/README
  ${CMAKE_CURRENT_SOURCE_DIR}/runtests
  ${CMAKE_CURRENT_SOURCE_DIR}/self.std
  ${CMAKE_CURRENT_SOURCE_DIR}/sndrcvrep.std
  ${CMAKE_CURRENT_SOURCE_DIR}/waitany.std
  ${CMAKE_CURRENT_SOURCE_DIR}/reqcreate.std
  ${CMAKE_CURRENT_SOURCE_DIR}/persistent.std
  ${CMAKE_CURRENT_SOURCE_DIR}/allpair2.std
  ${CMAKE_CURRENT_SOURCE_DIR}/typeub3.std
  ${CMAKE_CURRENT_SOURCE_DIR}/waitall.std
  ${CMAKE_CURRENT_SOURCE_DIR}/nullproc2.std
  ${CMAKE_CURRENT_SOURCE_DIR}/allpair.std
  ${CMAKE_CURRENT_SOURCE_DIR}/hindexed.std
  ${CMAKE_CURRENT_SOURCE_DIR}/ssendtest2.std
  ${CMAKE_CURRENT_SOURCE_DIR}/sendmany.std
  ${CMAKE_CURRENT_SOURCE_DIR}/nullproc.std
  ${CMAKE_CURRENT_SOURCE_DIR}/nbtest.std
  ${CMAKE_CURRENT_SOURCE_DIR}/sendcplx.std
  ${CMAKE_CURRENT_SOURCE_DIR}/hvec.std
  ${CMAKE_CURRENT_SOURCE_DIR}/typebase.std
  ${CMAKE_CURRENT_SOURCE_DIR}/typeub2.std
  ${CMAKE_CURRENT_SOURCE_DIR}/structf.std
  ${CMAKE_CURRENT_SOURCE_DIR}/typecreate.std
  ${CMAKE_CURRENT_SOURCE_DIR}/nblock.std
  PARENT_SCOPE
  )
