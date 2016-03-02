if(enable_smpi AND enable_smpi_MPICH3_testsuite)
  if(WIN32)
    set(CMAKE_C_FLAGS "-include ${CMAKE_HOME_DIRECTORY}/include/smpi/smpi_main.h")
  else()
    set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc")
    set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff")
  endif()

  set(MPICH_FLAGS "-DHAVE_STDLIB_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STRING_H=1 -DUSE_STDARG=1 -DHAVE_LONG_DOUBLE=1 -DHAVE_PROTOTYPES=1 -DHAVE_SIGNAL_H=1 -DHAVE_SIGACTION=1 -DHAVE_SLEEP=1 -DHAVE_SYSCONF=1  -Wno-error=unused-variable")
  include_directories(BEFORE "${CMAKE_HOME_DIRECTORY}/include/smpi")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

  add_executable(accfence1 accfence1.c)
  add_executable(accfence2_am accfence2_am.c)
  add_executable(accfence2 accfence2.c)
  add_executable(accpscw1 accpscw1.c)
#  add_executable(acc-loc acc-loc.c)
#  add_executable(adlb_mimic1 adlb_mimic1.c)
  add_executable(allocmem allocmem.c)
#  add_executable(attrorderwin attrorderwin.c)
#  add_executable(badrma badrma.c)
#  add_executable(baseattrwin baseattrwin.c)
#  add_executable(compare_and_swap compare_and_swap.c)
#  add_executable(contention_put contention_put.c)
#  add_executable(contention_putget contention_putget.c)
#  add_executable(contig_displ contig_displ.c)
  add_executable(epochtest epochtest.c)
#  add_executable(fence_shm fence_shm.c)
#  add_executable(fetchandadd_am fetchandadd_am.c)
#  add_executable(fetchandadd fetchandadd.c)
#  add_executable(fetchandadd_tree_am fetchandadd_tree_am.c)
#  add_executable(fetchandadd_tree fetchandadd_tree.c)
#  add_executable(fetch_and_op fetch_and_op.c)
#  add_executable(fkeyvalwin fkeyvalwin.c)
#  add_executable(flush flush.c)
#  add_executable(get_acc_local get_acc_local.c)
#  add_executable(get_accumulate get_accumulate.c)
  add_executable(getfence1 getfence1.c)
  add_executable(getgroup getgroup.c)
#  add_executable(ircpi ircpi.c)
#  add_executable(linked_list_bench_lock_all linked_list_bench_lock_all.c)
#  add_executable(linked_list_bench_lock_excl linked_list_bench_lock_excl.c)
#  add_executable(linked_list_bench_lock_shr linked_list_bench_lock_shr.c)
#  add_executable(linked_list linked_list.c)
#  add_executable(linked_list_fop linked_list_fop.c)
#  add_executable(linked_list_lockall linked_list_lockall.c)
#  add_executable(lockcontention2 lockcontention2.c)
#  add_executable(lockcontention3 lockcontention3.c)
#  add_executable(lockcontention lockcontention.c)
#  add_executable(locknull locknull.c)
#  add_executable(lockopts lockopts.c)
#  add_executable(manyrma2 manyrma2.c)
  add_executable(manyrma3 manyrma3.c)
#  add_executable(mcs-mutex mcs-mutex.c)
#  add_executable(mixedsync mixedsync.c)
#  add_executable(mutex_bench mutex_bench.c)
  add_executable(nullpscw nullpscw.c)
#  add_executable(pscw_ordering pscw_ordering.c)
#  add_executable(put_base put_base.c)
#  add_executable(put_bottom put_bottom.c)
  add_executable(putfence1 putfence1.c)
  add_executable(putfidx putfidx.c)
  add_executable(putpscw1 putpscw1.c)
#  add_executable(req_example req_example.c)
#  add_executable(reqops reqops.c)
#  add_executable(rmanull rmanull.c)
#  add_executable(rmazero rmazero.c)
#  add_executable(rma-contig rma-contig.c)
#  add_executable(selfrma selfrma.c)
#  add_executable(strided_acc_indexed strided_acc_indexed.c)
#  add_executable(strided_acc_onelock strided_acc_onelock.c)
#  add_executable(strided_acc_subarray strided_acc_subarray.c)
#  add_executable(strided_getacc_indexed strided_getacc_indexed.c)
#  add_executable(strided_getacc_indexed_shared strided_getacc_indexed_shared.c)
#  add_executable(strided_get_indexed strided_get_indexed.c)
#  add_executable(strided_putget_indexed strided_putget_indexed.c)
#  add_executable(strided_putget_indexed_shared strided_putget_indexed_shared.c)
  add_executable(test1_am test1_am.c)
  add_executable(test1 test1.c)
  add_executable(test1_dt test1_dt.c)
  add_executable(test2_am test2_am.c)
  add_executable(test2 test2.c)
#  add_executable(test3_am test3_am.c)
  add_executable(test3 test3.c)
#  add_executable(test4_am test4_am.c)
#  add_executable(test4 test4.c)
  add_executable(test5_am test5_am.c)
  add_executable(test5 test5.c)
  add_executable(transpose1 transpose1.c)
  add_executable(transpose2 transpose2.c)
  add_executable(transpose3 transpose3.c)
#  add_executable(transpose4 transpose4.c)
#  add_executable(transpose5 transpose5.c)
  add_executable(transpose6 transpose6.c)
  add_executable(transpose7 transpose7.c)
#  add_executable(wincall wincall.c)
  add_executable(window_creation window_creation.c)
#  add_executable(win_dynamic_acc win_dynamic_acc.c)
#  add_executable(win_flavors win_flavors.c)
#  add_executable(win_info win_info.c)
#  add_executable(winname winname.c)
#  add_executable(win_shared win_shared.c)
#  add_executable(win_shared_noncontig win_shared_noncontig.c)
#  add_executable(win_shared_noncontig_put win_shared_noncontig_put.c)
#  add_executable(win_large_shm win_large_shm.c)
#  add_executable(win_zero win_zero.c)
#  add_executable(wintest wintest.c)

  target_link_libraries(accfence1 simgrid mtest_c)
  target_link_libraries(accfence2_am simgrid mtest_c)
  target_link_libraries(accfence2 simgrid mtest_c)
  target_link_libraries(accpscw1 simgrid mtest_c)
#  target_link_libraries(acc-loc simgrid mtest_c)
#  target_link_libraries(adlb_mimic1 simgrid mtest_c)
  target_link_libraries(allocmem simgrid mtest_c)
#  target_link_libraries(attrorderwin simgrid mtest_c)
#  target_link_libraries(badrma simgrid mtest_c)
#  target_link_libraries(baseattrwin simgrid mtest_c)
#  target_link_libraries(compare_and_swap simgrid mtest_c)
#  target_link_libraries(contention_put simgrid mtest_c)
#  target_link_libraries(contention_putget simgrid mtest_c)
#  target_link_libraries(contig_displ simgrid mtest_c)
  target_link_libraries(epochtest simgrid mtest_c)
#  target_link_libraries(fence_shm simgrid mtest_c)
#  target_link_libraries(fetchandadd_am simgrid mtest_c)
#  target_link_libraries(fetchandadd simgrid mtest_c)
#  target_link_libraries(fetchandadd_tree_am simgrid mtest_c)
#  target_link_libraries(fetchandadd_tree simgrid mtest_c)
#  target_link_libraries(fetch_and_op simgrid mtest_c)
#  target_link_libraries(fkeyvalwin simgrid mtest_c)
#  target_link_libraries(flush simgrid mtest_c)
#  target_link_libraries(get_acc_local simgrid mtest_c)
#  target_link_libraries(get_accumulate simgrid mtest_c)
  target_link_libraries(getfence1 simgrid mtest_c)
  target_link_libraries(getgroup simgrid mtest_c)
#  target_link_libraries(ircpi simgrid mtest_c)
#  target_link_libraries(linked_list_bench_lock_all simgrid mtest_c)
#  target_link_libraries(linked_list_bench_lock_excl simgrid mtest_c)
#  target_link_libraries(linked_list_bench_lock_shr simgrid mtest_c)
#  target_link_libraries(linked_list simgrid mtest_c)
#  target_link_libraries(linked_list_fop simgrid mtest_c)
#  target_link_libraries(linked_list_lockall simgrid mtest_c)
#  target_link_libraries(lockcontention2 simgrid mtest_c)
#  target_link_libraries(lockcontention3 simgrid mtest_c)
#  target_link_libraries(lockcontention simgrid mtest_c)
#  target_link_libraries(locknull simgrid mtest_c)
#  target_link_libraries(lockopts simgrid mtest_c)
#  target_link_libraries(manyrma2 simgrid mtest_c)
  target_link_libraries(manyrma3 simgrid mtest_c)
#  target_link_libraries(mcs-mutex simgrid mtest_c)
#  target_link_libraries(mixedsync simgrid mtest_c)
#  target_link_libraries(mutex_bench simgrid mtest_c)
  target_link_libraries(nullpscw simgrid mtest_c)
#  target_link_libraries(pscw_ordering simgrid mtest_c)
#  target_link_libraries(put_base simgrid mtest_c)
#  target_link_libraries(put_bottom simgrid mtest_c)
  target_link_libraries(putfence1 simgrid mtest_c)
  target_link_libraries(putfidx simgrid mtest_c)
  target_link_libraries(putpscw1 simgrid mtest_c)
#  target_link_libraries(req_example simgrid mtest_c)
#  target_link_libraries(reqops simgrid mtest_c)
#  target_link_libraries(rma-contig simgrid mtest_c)
#  target_link_libraries(rmanull simgrid mtest_c)
#  target_link_libraries(rmazero simgrid mtest_c)
#  target_link_libraries(selfrma simgrid mtest_c)
#  target_link_libraries(strided_acc_indexed simgrid mtest_c)
#  target_link_libraries(strided_acc_onelock simgrid mtest_c)
#  target_link_libraries(strided_acc_subarray simgrid mtest_c)
#  target_link_libraries(strided_getacc_indexed simgrid mtest_c)
#  target_link_libraries(strided_getacc_indexed_shared simgrid mtest_c)
#  target_link_libraries(strided_get_indexed simgrid mtest_c)
#  target_link_libraries(strided_putget_indexed simgrid mtest_c)
#  target_link_libraries(strided_putget_indexed_shared simgrid mtest_c)
  target_link_libraries(test1_am simgrid mtest_c)
  target_link_libraries(test1 simgrid mtest_c)
  target_link_libraries(test1_dt simgrid mtest_c)
  target_link_libraries(test2_am simgrid mtest_c)
  target_link_libraries(test2 simgrid mtest_c)
#  target_link_libraries(test3_am simgrid mtest_c)
  target_link_libraries(test3 simgrid mtest_c)
#  target_link_libraries(test4_am simgrid mtest_c)
#  target_link_libraries(test4 simgrid mtest_c)
  target_link_libraries(test5_am simgrid mtest_c)
  target_link_libraries(test5 simgrid mtest_c)
  target_link_libraries(transpose1 simgrid mtest_c)
  target_link_libraries(transpose2 simgrid mtest_c)
  target_link_libraries(transpose3 simgrid mtest_c)
#  target_link_libraries(transpose4 simgrid mtest_c)
#  target_link_libraries(transpose5 simgrid mtest_c)
  target_link_libraries(transpose6 simgrid mtest_c)
  target_link_libraries(transpose7 simgrid mtest_c)
#  target_link_libraries(wincall simgrid mtest_c)
  target_link_libraries(window_creation simgrid mtest_c)
#  target_link_libraries(win_dynamic_acc simgrid mtest_c)
#  target_link_libraries(win_flavors simgrid mtest_c)
#  target_link_libraries(win_info simgrid mtest_c)
#  target_link_libraries(winname simgrid mtest_c)
#  target_link_libraries(win_shared simgrid mtest_c)
#  target_link_libraries(win_shared_noncontig simgrid mtest_c)
#  target_link_libraries(win_shared_noncontig_put simgrid mtest_c)
#  target_link_libraries(win_large_shm simgrid mtest_c)
#  target_link_libraries(win_zero simgrid mtest_c)
#  target_link_libraries(wintest simgrid mtest_c)

 set_target_properties(accfence1 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(accfence2_am PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(accfence2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(acc-loc PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(accpscw1 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(adlb_mimic1 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(allocmem PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(attrorderwin PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(badrma PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(baseattrwin PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(compare_and_swap PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(contention_put PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(contention_putget PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(contig_displ PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(epochtest PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(fence_shm PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(fetchandadd_am PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(fetchandadd PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(fetchandadd_tree_am PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(fetchandadd_tree PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(fetch_and_op PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(fkeyvalwin PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(flush PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(get_acc_local PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(get_accumulate PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(getfence1 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(getgroup PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(ircpi PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(linked_list_bench_lock_all PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(linked_list_bench_lock_excl PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(linked_list_bench_lock_shr PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(linked_list PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(linked_list_fop PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(linked_list_lockall PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(lockcontention2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(lockcontention3 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(lockcontention PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(locknull PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(lockopts PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(manyrma2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(manyrma3 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(mcs-mutex PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(mixedsync PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(mutex_bench PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(nullpscw PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(pscw_ordering PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(put_base PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(put_bottom PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(putfence1 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(putfidx PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(putpscw1 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(req_example PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(reqops PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(rmanull PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(rmazero PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(rma-contig PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(selfrma PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(strided_acc_indexed PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(strided_acc_onelock PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(strided_acc_subarray PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(strided_getacc_indexed PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(strided_getacc_indexed_shared PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(strided_get_indexed PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(strided_putget_indexed PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(strided_putget_indexed_shared PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(test1_am PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(test1 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(test1_dt PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(test2_am PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(test2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(test3_am PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(test3 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(test4_am PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(test4 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(test5_am PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(test5 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(transpose1 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(transpose2 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(transpose3 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(transpose4 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(transpose5 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(transpose6 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(transpose7 PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(wincall PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
 set_target_properties(window_creation PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(win_dynamic_acc PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(win_flavors PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(win_info PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(winname PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(win_shared PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(win_shared_noncontig PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(win_shared_noncontig_put PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(win_large_shm PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(win_zero PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
# set_target_properties(wintest PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
endif()

set(examples_src
  ${examples_src}
 ${CMAKE_CURRENT_SOURCE_DIR}/accfence1.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/accfence2_am.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/accfence2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/acc-loc.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/accpscw1.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/adlb_mimic1.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/allocmem.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/attrorderwin.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/badrma.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/baseattrwin.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/compare_and_swap.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/contention_put.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/contention_putget.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/contig_displ.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/epochtest.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/fence_shm.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/fetchandadd_am.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/fetchandadd.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/fetchandadd_tree_am.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/fetchandadd_tree.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/fetch_and_op.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/fkeyvalwin.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/flush.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/get_acc_local.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/get_accumulate.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/getfence1.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/getgroup.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/ircpi.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/linked_list_bench_lock_all.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/linked_list_bench_lock_excl.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/linked_list_bench_lock_shr.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/linked_list.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/linked_list_fop.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/linked_list_lockall.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/lockcontention2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/lockcontention3.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/lockcontention.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/locknull.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/lockopts.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/manyrma2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/manyrma3.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/mcs-mutex.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/mixedsync.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/mutex_bench.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/nullpscw.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/pscw_ordering.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/put_base.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/put_bottom.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/putfence1.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/putfidx.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/putpscw1.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/req_example.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/reqops.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/rmanull.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/rmazero.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/rma-contig.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/selfrma.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/strided_acc_indexed.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/strided_acc_onelock.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/strided_acc_subarray.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/strided_getacc_indexed.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/strided_getacc_indexed_shared.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/strided_get_indexed.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/strided_putget_indexed.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/strided_putget_indexed_shared.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/test1_am.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/test1.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/test1_dt.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/test2_am.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/test2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/test3_am.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/test3.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/test4_am.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/test4.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/test5_am.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/test5.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/transpose1.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/transpose2.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/transpose3.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/transpose4.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/transpose5.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/transpose6.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/transpose7.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/wincall.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/window_creation.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/win_dynamic_acc.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/win_flavors.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/win_info.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/win_large_shm.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/win_zero.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/winname.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/win_shared.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/win_shared_noncontig.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/win_shared_noncontig_put.c 
 ${CMAKE_CURRENT_SOURCE_DIR}/wintest.c 
  PARENT_SCOPE)
set(txt_files
  ${txt_files}
  ${CMAKE_CURRENT_SOURCE_DIR}/testlist
  ${CMAKE_CURRENT_SOURCE_DIR}/mcs-mutex.h
  ${CMAKE_CURRENT_SOURCE_DIR}/squelch.h
  PARENT_SCOPE)
