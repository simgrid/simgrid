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

  foreach(file accfence1 accfence2_am accfence2 accpscw1 allocmem epochtest getfence1 getgroup manyrma3 nullpscw lockcontention lockopts contig_displ
          putfence1 putfidx putpscw1 test1_am test1 test1_dt test2_am test2 test3 test3_am test4 test4_am test5_am test5 transpose1 transpose2 lockcontention2 wincall
          transpose3 transpose4 transpose5 transpose6 transpose7 window_creation selfrma locknull win_info
          at_complete  acc-pairtype manyget large-small-acc lock_dt win_dynamic_acc fetch_and_op flush req_example rmanull rmazero badrma
          lock_nested winname attrorderwin baseattrwin fkeyvalwin strided_acc_indexed strided_getacc_indexed compare_and_swap
          lockall_dt lockall_dt_flushall lock_dt_flush lockall_dt_flush lockall_dt_flushlocalall lockall_dt_flushlocal  lock_dt_flushlocal
     strided_acc_onelock strided_get_indexed strided_putget_indexed contention_put contention_putget lockcontention3
     adlb_mimic1 lock_contention_dt acc-loc get_acc_local get_accumulate put_base put_bottom
     linked_list_bench_lock_all linked_list_bench_lock_excl manyrma2 pscw_ordering rma-contig get-struct
     rput_local_comp racc_local_comp rget-testall win_shared win_shared_put_flush_get atomic_get)
    # fence_shm fetchandadd_am fetchandadd fetchandadd_tree_am fetchandadd_tree
    # linked_list_bench_lock_shr linked_list linked_list_fop linked_list_lockall
    #   mcs-mutex mixedsync mutex_bench  reqops
    # strided_getacc_indexed_shared rget-unlock
    # win_flavors win_shared_zerobyte win_shared_noncontig win_shared_noncontig_put
    # win_large_shm win_zero wintest  atomic_rmw_fop atomic_rmw_gacc atomic_rmw_cas
    # aint derived-acc-flush_local large-acc-flush_local
    #  win_shared_create win_shared_put_flush_get win_shared_rma_flush_load
    # wrma_flush_get
    add_executable(${file} EXCLUDE_FROM_ALL ${file}.c)
    add_dependencies(tests ${file})
    target_link_libraries(${file} simgrid mtest_c)
    set_target_properties(${file} PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS}")
  endforeach()

    add_executable(transpose3_shm EXCLUDE_FROM_ALL transpose3.c)
    add_dependencies(tests transpose3_shm)
    target_link_libraries(transpose3_shm simgrid mtest_c)
    set_target_properties(transpose3_shm PROPERTIES COMPILE_FLAGS "${MPICH_FLAGS} -DUSE_WIN_ALLOCATE")
endif()

if (enable_smpi_MPICH3_testsuite AND HAVE_RAW_CONTEXTS)
  ADD_TEST(test-smpi-mpich3-rma-raw       ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/rma ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests "-wrapper=${TESH_WRAPPER}" -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/rma -tests=testlist -execarg=--cfg=contexts/factory:raw)
  SET_TESTS_PROPERTIES(test-smpi-mpich3-rma-raw PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
  if (enable_thread_sanitizer)
    SET_TESTS_PROPERTIES(test-smpi-mpich3-rma-raw PROPERTIES TIMEOUT 1500)
  endif()
endif()

foreach(file accfence1 accfence2_am accfence2 accpscw1 allocmem epochtest getfence1 getgroup manyrma3 nullpscw
          putfence1 putfidx putpscw1 test1_am test1 test1_dt test2_am test2 test3 test5_am test5 transpose1 transpose2
          transpose3 transpose6 transpose7 window_creation
          acc-loc adlb_mimic1 attrorderwin badrma baseattrwin compare_and_swap contention_put contention_putget
          contig_displ fence_shm fetchandadd_am fetchandadd fetchandadd_tree_am fetchandadd_tree fetch_and_op
          fkeyvalwin flush get_acc_local get_accumulate linked_list_bench_lock_all linked_list_bench_lock_excl
          linked_list_bench_lock_shr linked_list linked_list_fop linked_list_lockall lockcontention2 lockcontention3
          lockcontention locknull lockopts manyrma2 mcs-mutex mixedsync mutex_bench pscw_ordering put_base put_bottom
          req_example reqops rmanull rmazero rma-contig selfrma strided_acc_indexed strided_acc_onelock
          strided_acc_subarray strided_getacc_indexed strided_getacc_indexed_shared strided_get_indexed
          strided_putget_indexed strided_putget_indexed_shared test3_am test4_am test4 transpose4 transpose5 wincall
          win_dynamic_acc win_flavors win_info winname win_shared win_shared_noncontig win_shared_noncontig_put
          win_large_shm win_zero wintest win_shared_zerobyte get-struct
          at_complete atomic_rmw_fop atomic_rmw_cas atomic_rmw_gacc atomic_get aint acc-pairtype manyget large-small-acc
          rget-unlock overlap_wins_rma lock_nested derived-acc-flush_local large-acc-flush_local lockall_dt lockall_dt_flushall
          lockall_dt_flush nb_test lockall_dt_flushlocal lockall_dt_flushlocalall lock_contention_dt lock_dt lock_dt_flush lock_dt_flushlocal
          racc_local_comp rput_local_comp win_shared_create win_shared_put_flush_get win_shared_rma_flush_load
          wrma_flush_get rget-testall)
  set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${file}.c)
endforeach()

set(examples_src  ${examples_src}                                           PARENT_SCOPE)
set(txt_files     ${txt_files}     ${CMAKE_CURRENT_SOURCE_DIR}/testlist
                                   ${CMAKE_CURRENT_SOURCE_DIR}/mcs-mutex.h
                                   ${CMAKE_CURRENT_SOURCE_DIR}/squelch.h    PARENT_SCOPE)
