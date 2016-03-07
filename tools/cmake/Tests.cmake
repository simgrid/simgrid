IF(enable_smpi AND NOT WIN32)
  exec_program("chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpicc" OUTPUT_VARIABLE "OKITOKI")
  exec_program("chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpicxx" OUTPUT_VARIABLE "OKITOKI")
  exec_program("chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpiff" OUTPUT_VARIABLE "OKITOKI")
  exec_program("chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpif90" OUTPUT_VARIABLE "OKITOKI")
  exec_program("chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpirun" OUTPUT_VARIABLE "OKITOKI")
ENDIF()

SET(TESH_COMMAND ${PERL_EXECUTABLE} ${CMAKE_BINARY_DIR}/bin/tesh)
IF(CMAKE_HOST_WIN32)
  SET(TESH_OPTION $TESH_OPTION --timeout 50)
ENDIF()

#some tests may take forever on non futexes systems, using busy_wait with n cores < n workers
# default to posix for these tests if futexes are not supported
IF(NOT HAVE_FUTEX_H)
  SET(CONTEXTS_SYNCHRO --cfg contexts/synchro:posix)
ENDIF()

MACRO(ADD_TESH NAME)
  SET(ARGT ${ARGV})
  LIST(REMOVE_AT ARGT 0)
  IF(WIN32)
    STRING(REPLACE "§" "\;" ARGT "${ARGT}")
  ENDIF()
  ADD_TEST(${NAME} ${TESH_COMMAND} ${TESH_OPTION} ${ARGT})
ENDMACRO()

MACRO(ADD_TESH_FACTORIES NAME FACTORIES)
  SET(ARGR ${ARGV})
  LIST(REMOVE_AT ARGR 0) # remove name
  FOREACH(I ${FACTORIES}) # remove all factories
    LIST(REMOVE_AT ARGR 0)
  ENDFOREACH()
  FOREACH(FACTORY ${FACTORIES})
    if ((${FACTORY} STREQUAL "thread" AND HAVE_THREAD_CONTEXTS) OR
        (${FACTORY} STREQUAL "boost" AND HAVE_BOOST_CONTEXTS) OR
        (${FACTORY} STREQUAL "raw" AND HAVE_RAW_CONTEXTS) OR
        (${FACTORY} STREQUAL "ucontext" AND HAVE_UCONTEXT_CONTEXTS))
      ADD_TESH("${NAME}-${FACTORY}" "--cfg" "contexts/factory:${FACTORY}" ${ARGR})
    ENDIF()
  ENDFOREACH()
ENDMACRO()

IF(NOT enable_memcheck)
  ### GENERIC  ###
  # BEGIN TESH TESTS
  # test for code coverage
    ADD_TEST(test-help                             ${CMAKE_BINARY_DIR}/teshsuite/simdag/platforms/basic_parsing_test --help)
    ADD_TEST(test-help-models                      ${CMAKE_BINARY_DIR}/teshsuite/simdag/platforms/basic_parsing_test --help-models)
    ADD_TEST(test-tracing-help                   ${CMAKE_BINARY_DIR}/teshsuite/simdag/platforms/basic_parsing_test --help-tracing)
    ADD_TESH(graphicator                         --setenv srcdir=${CMAKE_HOME_DIRECTORY} --setenv bindir=${CMAKE_BINARY_DIR}/bin --cd ${CMAKE_HOME_DIRECTORY}/tools/graphicator graphicator.tesh)
  # END TESH TESTS

  ADD_TESH(mc-replay-random-bug                  --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/replay --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/mc/replay --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/replay random_bug_replay.tesh)

  ### MC ###
  IF(HAVE_MC)
    ADD_TESH(tesh-mc-dwarf                       --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/dwarf --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/dwarf dwarf.tesh)
    ADD_TESH(tesh-mc-dwarf-expression            --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/dwarf_expression --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/dwarf_expression dwarf_expression.tesh)

    ADD_TESH(tesh-mc-with-mutex-handling              --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc with_mutex_handling.tesh --cfg=model-check/reduction:none)
#    ADD_TESH(tesh-mc-with-mutex-handling-dpor         --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc with_mutex_handling.tesh --cfg=model-check/reduction:dpor)
    ADD_TESH(tesh-mc-without-mutex-handling           --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc without_mutex_handling.tesh --cfg=model-check/reduction:none)
    ADD_TESH(tesh-mc-without-mutex-handling-dpor      --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc without_mutex_handling.tesh --cfg=model-check/reduction:dpor)

    ADD_TESH(mc-record-random-bug                --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/mc/replay --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/mc/replay --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/mc/replay random_bug.tesh)

    ADD_TESH_FACTORIES(mc-bugged1                "ucontext;raw" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/mc --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/mc bugged1.tesh)
    ADD_TESH_FACTORIES(mc-bugged2                "ucontext;raw" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/mc --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/mc bugged2.tesh)
    IF(HAVE_UCONTEXT_CONTEXTS AND PROCESSOR_x86_64) # liveness model-checking works only on 64bits (for now ...)
      ADD_TESH(mc-bugged1-liveness-ucontext         --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/mc --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/mc bugged1_liveness.tesh)
      ADD_TESH(mc-bugged1-liveness-ucontext-sparse  --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/mc --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/mc bugged1_liveness_sparse.tesh)
      ADD_TESH(mc-bugged1-liveness-visited-ucontext --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/mc --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/mc bugged1_liveness_visited.tesh)
      ADD_TESH(mc-bugged1-liveness-visited-ucontext-sparse --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/mc --cd ${CMAKE_HOME_DIRECTORY}/examples/msg/mc bugged1_liveness_visited_sparse.tesh)
    if(HAVE_C_STACK_CLEANER)
      # This test checks if the stack cleaner is makign a difference:
      add_test(mc-bugged1-liveness-stack-cleaner
        ${CMAKE_HOME_DIRECTORY}/examples/msg/mc/bugged1_liveness_stack_cleaner
        ${CMAKE_HOME_DIRECTORY}/examples/msg/mc/
        ${CMAKE_BINARY_DIR}/examples/msg/mc/
        )
    endif()
    ENDIF()
  ENDIF()

  ### SIMIX ###
  # BEGIN TESH TESTS
  IF(HAVE_RAW_CONTEXTS)
    ADD_TESH(tesh-simix-factory-default          --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simix/check_defaults --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simix/check_defaults factory_raw.tesh)
  ELSEIF(HAVE_UCONTEXT_CONTEXTS)
    ADD_TESH(tesh-simix-factory-default          --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simix/check_defaults --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simix/check_defaults factory_ucontext.tesh)
  ELSE()
    ADD_TESH(tesh-simix-factory-default          --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simix/check_defaults --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simix/check_defaults factory_thread.tesh)
  ENDIF()
  IF(HAVE_THREAD_CONTEXTS)
  ADD_TESH(tesh-simix-factory-thread             --cfg contexts/factory:thread --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simix/check_defaults --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simix/check_defaults factory_thread.tesh)
  ENDIF()
  IF(HAVE_RAW_CONTEXTS)
    ADD_TESH(tesh-simix-factory-raw              --cfg contexts/factory:raw --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simix/check_defaults --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simix/check_defaults factory_raw.tesh)
  ENDIF()
  IF(HAVE_UCONTEXT_CONTEXTS)
    ADD_TESH(tesh-simix-factory-ucontext         --cfg contexts/factory:ucontext --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simix/check_defaults --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simix/check_defaults factory_ucontext.tesh)
  ENDIF()
  # END TESH TESTS
  ADD_TESH_FACTORIES(stack-overflow              "thread;ucontext;raw" --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simix/stack_overflow --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simix/stack_overflow stack_overflow.tesh)
  ###
  ### Declare that we know that some tests are broken
  ###
  IF(release)
    IF(WIN32 OR CMAKE_SYSTEM_NAME MATCHES "Darwin")
      # These tests are known to fail on Windows and Mac OS X
      # (the expected error message is not shown).
      IF(HAVE_THREAD_CONTEXTS)
      SET_TESTS_PROPERTIES(stack-overflow-thread PROPERTIES WILL_FAIL true)
      ENDIF()
      IF(HAVE_UCONTEXT_CONTEXTS)
        SET_TESTS_PROPERTIES(stack-overflow-ucontext PROPERTIES WILL_FAIL true)
      ENDIF()
      IF(HAVE_RAW_CONTEXTS)
        SET_TESTS_PROPERTIES(stack-overflow-raw PROPERTIES WILL_FAIL true)
      ENDIF()
    ENDIF()
  ENDIF()

  ## INTERFACES ##
  ### MSG ###
  # BEGIN TESH TESTS
  ADD_TESH_FACTORIES(msg-masterslave-kill        "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave/masterslave_kill.tesh)
  ADD_TESH_FACTORIES(msg-masterslave-multicore   "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave/masterslave_multicore.tesh)
  ADD_TESH_FACTORIES(msg-masterslave-no-crosstraffic-mailbox   "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave/masterslave_mailbox.tesh)
  ADD_TESH_FACTORIES(msg-masterslave-no-crosstraffic           "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave/masterslave.tesh)
  ADD_TESH_FACTORIES(msg-masterslave-no-crosstraffic-forwarder "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave/masterslave_forwarder.tesh)
  ADD_TESH_FACTORIES(msg-masterslave-no-crosstraffic-failure   "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave/masterslave_failure.tesh)
  ADD_TESH_FACTORIES(msg-masterslave             "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave/masterslave_crosstraffic.tesh)
  ADD_TESH_FACTORIES(msg-masterslave-forwarder   "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave/masterslave_forwarder_crosstraffic.tesh)
  ADD_TESH_FACTORIES(msg-masterslave-failure     "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave/masterslave_failure_crosstraffic.tesh)
  ADD_TESH_FACTORIES(msg-masterslave-mailbox     "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave/masterslave_mailbox_crosstraffic.tesh)
  ADD_TESH_FACTORIES(msg-masterslave-cpu-ti      "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg/masterslave --cd ${CMAKE_HOME_DIRECTORY}/examples/msg masterslave/masterslave_cpu_ti_crosstraffic.tesh)
  ADD_TESH_FACTORIES(msg-masterslave-vivaldi     "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/msg --cd ${CMAKE_BINARY_DIR}/examples/msg ${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave/masterslave_vivaldi.tesh)
  
  ADD_TESH(tracing-ms                          --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg --cd ${CMAKE_HOME_DIRECTORY}/examples/msg tracing/ms.tesh)
  ADD_TESH(tracing-trace-platform              --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg --cd ${CMAKE_HOME_DIRECTORY}/examples/msg tracing/trace_platform.tesh)
  ADD_TESH(tracing-user-variables              --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg --cd ${CMAKE_HOME_DIRECTORY}/examples/msg tracing/user_variables.tesh)
  ADD_TESH(tracing-link-user-variables         --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg --cd ${CMAKE_HOME_DIRECTORY}/examples/msg tracing/link_user_variables.tesh)
  ADD_TESH(tracing-link-srcdst-user-variables  --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg --cd ${CMAKE_HOME_DIRECTORY}/examples/msg tracing/link_srcdst_user_variables.tesh)
  ADD_TESH(tracing-categories                  --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg --cd ${CMAKE_HOME_DIRECTORY}/examples/msg tracing/categories.tesh)
  ADD_TESH(tracing-process-migration           --setenv bindir=${CMAKE_BINARY_DIR}/examples/msg --cd ${CMAKE_HOME_DIRECTORY}/examples/msg tracing/procmig.tesh)
  # END TESH TESTS

  ### S4U ###
  ADD_TESH_FACTORIES(s4u-basic "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/examples/s4u/basic --cd ${CMAKE_HOME_DIRECTORY}/examples/s4u/basic s4u_basic.tesh)
  ADD_TESH_FACTORIES(s4u-io "thread;ucontext;raw;boost"    --setenv bindir=${CMAKE_BINARY_DIR}/examples/s4u/io --cd ${CMAKE_HOME_DIRECTORY}/examples/s4u/io s4u_io.tesh)

  ### SIMDAG ###
  # BEGIN TESH TESTS
  # these tests need the assertion mechanism
  # exclude them from memcheck, as they normally die, leaving lots of unfree'd objects
  IF(enable_debug AND NOT enable_memcheck)
    ADD_TESH(tesh-parser-bogus-symmetric         --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simdag/platforms --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/platforms bogus_two_hosts_asymetric.tesh)
    ADD_TESH(tesh-parser-bogus-missing-gw        --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simdag/platforms --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/platforms bogus_missing_gateway.tesh)
    ADD_TESH(tesh-parser-bogus-disk-attachment   --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simdag/platforms --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/platforms bogus_disk_attachment.tesh)

    #These tests fail on Windows as the signal returned is not the same as Unix would send.
    IF(WIN32)
      set_property(TEST tesh-parser-bogus-symmetric PROPERTY WILL_FAIL TRUE)
      set_property(TEST tesh-parser-bogus-missing-gw PROPERTY WILL_FAIL TRUE)
      set_property(TEST tesh-parser-bogus-disk-attachment PROPERTY WILL_FAIL TRUE)
    ENDIF()
  ENDIF()
  ADD_TESH(tesh-simdag-bypass                    --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simdag/platforms --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/platforms basic_parsing_test_bypass.tesh)
  ADD_TESH(tesh-simdag-flatifier                 --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simdag/platforms --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/platforms flatifier.tesh)
  ADD_TESH(tesh-simdag-link                      --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simdag/platforms --setenv srcdir=${CMAKE_HOME_DIRECTORY} --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/platforms basic_link_test.tesh)
  ADD_TESH(tesh-simdag-parser                    --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simdag/platforms --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/platforms basic_parsing_test.tesh)
  ADD_TESH(tesh-simdag-parser-sym-full           --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simdag/platforms --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/platforms basic_parsing_test_sym_full.tesh)
  ADD_TESH(tesh-simdag-full-links                --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/simdag/platforms --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/platforms get_full_link.tesh)
  ADD_TEST(tesh-simdag-full-links01                ${CMAKE_BINARY_DIR}/teshsuite/simdag/platforms/basic_parsing_test ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/platforms/two_clusters.xml FULL_LINK)
  ADD_TEST(tesh-simdag-full-links02                ${CMAKE_BINARY_DIR}/teshsuite/simdag/platforms/basic_parsing_test ${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/platforms/two_clusters_one_name.xml FULL_LINK)
  ADD_TEST(tesh-simdag-one-link-g5k                ${CMAKE_BINARY_DIR}/teshsuite/simdag/platforms/basic_parsing_test ${CMAKE_HOME_DIRECTORY}/examples/platforms/g5k.xml ONE_LINK)
  # END TESH TESTS

  ### SMPI ###
  IF(enable_smpi)
    # BEGIN TESH TESTS
    # smpi examples
    ADD_TESH_FACTORIES(tesh-smpi-reduce          "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/reduce --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/reduce reduce.tesh)
    ADD_TESH_FACTORIES(tesh-smpi-compute         "thread;ucontext;raw;boost" --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/compute --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/compute compute.tesh)
    FOREACH (ALLREDUCE_COLL default lr rab1 rab2 rab_rdb
                            rdb smp_binomial smp_binomial_pipeline
                            smp_rdb smp_rsag smp_rsag_lr smp_rsag_rab redbcast ompi mpich ompi_ring_segmented mvapich2 mvapich2_rs mvapich2_two_level impi)
      ADD_TESH(tesh-smpi-allreduce-coll-${ALLREDUCE_COLL} --cfg smpi/allreduce:${ALLREDUCE_COLL} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/allreduce --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/allreduce allreduce_coll.tesh)
    ENDFOREACH()
    FOREACH (ALLREDUCE_COLL_LARGE ompi_ring_segmented)
      ADD_TESH(tesh-smpi-allreduce-coll-large-${ALLREDUCE_COLL_LARGE} --cfg smpi/allreduce:${ALLREDUCE_COLL_LARGE} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/allreduce --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/allreduce allreduce_coll_large.tesh)
    ENDFOREACH()
    ADD_TESH(tesh-smpi-allreduce-coll-automatic --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/allreduce --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/allreduce allreduce_coll_automatic.tesh)
    FOREACH (ALLTOALL_COLL 2dmesh 3dmesh pair pair_rma pair_one_barrier pair_light_barrier
                           pair_mpi_barrier rdb ring ring_light_barrier
                           ring_mpi_barrier ring_one_barrier
                           bruck basic_linear ompi mpich mvapich2 mvapich2_scatter_dest, impi)
      ADD_TESH(tesh-smpi-alltoall-coll-${ALLTOALL_COLL} --cfg smpi/alltoall:${ALLTOALL_COLL} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/alltoall --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/alltoall alltoall_coll.tesh)
    ENDFOREACH()   
    FOREACH (REDUCE_COLL default arrival_pattern_aware binomial flat_tree NTSL scatter_gather ompi mpich ompi_chain ompi_binary ompi_basic_linear ompi_binomial ompi_in_order_binary mvapich2 mvapich2_knomial mvapich2_two_level impi rab)
      ADD_TESH(tesh-smpi-reduce-coll-${REDUCE_COLL} --cfg smpi/reduce:${REDUCE_COLL} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/reduce --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/reduce reduce_coll.tesh)
    ENDFOREACH()
    FOREACH (REDUCE_SCATTER_COLL default  ompi mpich ompi_basic_recursivehalving ompi_ring mpich_noncomm mpich_pair mvapich2 mpich_rdb impi)
      ADD_TESH(tesh-smpi-reduce-scatter-coll-${REDUCE_SCATTER_COLL} --cfg smpi/reduce_scatter:${REDUCE_SCATTER_COLL} --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/reduce --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/reduce reduce_scatter_coll.tesh)
    ENDFOREACH()
      ADD_TESH(tesh-smpi-clusters-types --cfg smpi/alltoall:mvapich2 --setenv bindir=${CMAKE_BINARY_DIR}/teshsuite/smpi/alltoall --cd ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/alltoall clusters.tesh)
    # END TESH TESTS
    IF(enable_smpi_MPICH3_testsuite)
      IF(HAVE_THREAD_CONTEXTS)
        ADD_TEST(test-smpi-mpich3-coll-thread      ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/coll ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/coll -tests=testlist -execarg=--cfg=contexts/factory:thread -execarg=--cfg=smpi/privatize_global_variables:${HAVE_PRIVATIZATION})
        SET_TESTS_PROPERTIES(test-smpi-mpich3-coll-thread    PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
      ENDIF()
      IF(HAVE_UCONTEXT_CONTEXTS)
        ADD_TEST(test-smpi-mpich3-coll-ompi-ucontext ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/coll ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/coll -tests=testlist -execarg=--cfg=contexts/factory:ucontext -execarg=--cfg=smpi/coll_selector:ompi -execarg=--cfg=smpi/privatize_global_variables:${HAVE_PRIVATIZATION} -execarg=--cfg=smpi/bcast:binomial_tree)
        SET_TESTS_PROPERTIES(test-smpi-mpich3-coll-ompi-ucontext PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
      ENDIF()
      IF(HAVE_RAW_CONTEXTS)
        ADD_TEST(test-smpi-mpich3-coll-mpich-raw   ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/coll ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/coll -tests=testlist -execarg=--cfg=contexts/factory:raw -execarg=--cfg=smpi/coll_selector:mpich -execarg=--cfg=smpi/privatize_global_variables:${HAVE_PRIVATIZATION})
      ADD_TEST(test-smpi-mpich3-coll-ompi-raw ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/coll ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/coll -tests=testlist -execarg=--cfg=contexts/factory:raw -execarg=--cfg=smpi/coll_selector:ompi -execarg=--cfg=smpi/privatize_global_variables:${HAVE_PRIVATIZATION} -execarg=--cfg=smpi/bcast:binomial_tree)
      ADD_TEST(test-smpi-mpich3-coll-mpich-raw ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/coll ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/coll -tests=testlist -execarg=--cfg=contexts/factory:raw -execarg=--cfg=smpi/coll_selector:mpich -execarg=--cfg=smpi/privatize_global_variables:${HAVE_PRIVATIZATION})
      ADD_TEST(test-smpi-mpich3-coll-mvapich2-raw ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/coll ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/coll -tests=testlist -execarg=--cfg=contexts/factory:raw -execarg=--cfg=smpi/coll_selector:mvapich2 -execarg=--cfg=smpi/privatize_global_variables:${HAVE_PRIVATIZATION})
      ADD_TEST(test-smpi-mpich3-coll-impi-raw ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/coll ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/coll -tests=testlist -execarg=--cfg=contexts/factory:raw -execarg=--cfg=smpi/coll_selector:impi -execarg=--cfg=smpi/privatize_global_variables:${HAVE_PRIVATIZATION})
        SET_TESTS_PROPERTIES(test-smpi-mpich3-coll-mpich-raw test-smpi-mpich3-coll-ompi-raw test-smpi-mpich3-coll-mpich-raw test-smpi-mpich3-coll-mvapich2-raw test-smpi-mpich3-coll-impi-raw  PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
      ENDIF()
      IF(HAVE_RAW_CONTEXTS)
        ADD_TEST(test-smpi-mpich3-attr-raw       ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/attr ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/attr -tests=testlist -execarg=--cfg=contexts/factory:raw)
        ADD_TEST(test-smpi-mpich3-comm-raw       ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/comm ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/comm -tests=testlist -execarg=--cfg=contexts/factory:raw)
        ADD_TEST(test-smpi-mpich3-init-raw       ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/init ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/init -tests=testlist -execarg=--cfg=contexts/factory:raw)
        ADD_TEST(test-smpi-mpich3-datatype-raw   ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/datatype ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/datatype -tests=testlist -execarg=--cfg=contexts/factory:raw -execarg=--cfg=smpi/privatize_global_variables:${HAVE_PRIVATIZATION})
        ADD_TEST(test-smpi-mpich3-group-raw      ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/group ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/group -tests=testlist -execarg=--cfg=contexts/factory:raw)
        ADD_TEST(test-smpi-mpich3-pt2pt-raw      ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/pt2pt ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/pt2pt -tests=testlist -execarg=--cfg=contexts/factory:raw -execarg=--cfg=smpi/privatize_global_variables:${HAVE_PRIVATIZATION})
        ADD_TEST(test-smpi-mpich3-topo-raw       ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/topo ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/topo -tests=testlist -execarg=--cfg=contexts/factory:raw)
        ADD_TEST(test-smpi-mpich3-rma-raw       ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/rma ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/rma -tests=testlist -execarg=--cfg=contexts/factory:raw -execarg=--cfg=smpi/privatize_global_variables:${HAVE_PRIVATIZATION})
        ADD_TEST(test-smpi-mpich3-info-raw       ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/info ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/info -tests=testlist -execarg=--cfg=contexts/factory:raw -execarg=--cfg=smpi/privatize_global_variables:${HAVE_PRIVATIZATION})
        ADD_TEST(test-smpi-mpich3-perf-raw       ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/perf ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/perf -tests=testlist -execarg=--cfg=contexts/factory:raw -execarg=--cfg=smpi/running_power:-1)
        SET_TESTS_PROPERTIES(test-smpi-mpich3-attr-raw test-smpi-mpich3-comm-raw test-smpi-mpich3-init-raw test-smpi-mpich3-datatype-raw test-smpi-mpich3-group-raw test-smpi-mpich3-pt2pt-raw test-smpi-mpich3-topo-raw test-smpi-mpich3-rma-raw test-smpi-mpich3-info-raw PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
      ENDIF()
      IF(SMPI_FORTRAN AND HAVE_THREAD_CONTEXTS)
        ADD_TEST(test-smpi-mpich3-thread-f77     ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/f77/ ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/f77/ -tests=testlist -execarg=--cfg=contexts/stack_size:8000 -execarg=--cfg=smpi/privatize_global_variables:${HAVE_PRIVATIZATION})
        SET_TESTS_PROPERTIES(test-smpi-mpich3-thread-f77 PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
        ADD_TEST(test-smpi-mpich3-thread-f90     ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/teshsuite/smpi/mpich3-test/f90/ ${PERL_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/runtests -mpiexec=${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun -srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/mpich3-test/f90/ -tests=testlist -execarg=--cfg=smpi/privatize_global_variables:${HAVE_PRIVATIZATION})
        SET_TESTS_PROPERTIES(test-smpi-mpich3-thread-f90 PROPERTIES PASS_REGULAR_EXPRESSION "tests passed!")
      ENDIF()
    ENDIF()

    # BEGIN TESH TESTS
    ADD_TESH_FACTORIES(smpi-energy               "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi/energy --cd ${CMAKE_BINARY_DIR}/examples/smpi/energy ${CMAKE_HOME_DIRECTORY}/examples/smpi/energy/energy.tesh)
    IF(SMPI_FORTRAN)
      ADD_TESH_FACTORIES(smpi-energy-f77         "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi/energy --cd ${CMAKE_BINARY_DIR}/examples/smpi/energy ${CMAKE_HOME_DIRECTORY}/examples/smpi/energy/f77/energy.tesh)
      ADD_TESH_FACTORIES(smpi-energy-f90         "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi/energy --cd ${CMAKE_BINARY_DIR}/examples/smpi/energy ${CMAKE_HOME_DIRECTORY}/examples/smpi/energy/f90/energy.tesh)
    ENDIF()
    ADD_TESH_FACTORIES(smpi-msg-masterslave      "thread;ucontext;raw;boost" --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi/smpi_msg_masterslave --cd ${CMAKE_BINARY_DIR}/examples/smpi/smpi_msg_masterslave ${CMAKE_HOME_DIRECTORY}/examples/smpi/smpi_msg_masterslave/msg_smpi.tesh)
    IF(NOT HAVE_MC)
      ADD_TESH(smpi-replay-multiple      --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple --cd ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple ${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/replay_multiple.tesh)
    ENDIF()
    ADD_TESH(smpi-tracing-ptp                  --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi --cd ${CMAKE_BINARY_DIR}/examples/smpi ${CMAKE_HOME_DIRECTORY}/examples/smpi/tracing/smpi_traced.tesh)
    ADD_TESH(smpi-replay-simple                --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi --cd ${CMAKE_BINARY_DIR}/examples/smpi ${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/smpi_replay.tesh)
    IF(HAVE_MC)
      ADD_TESH(smpi-mc-only-send-determinism                  --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/smpi/mc --cd ${CMAKE_BINARY_DIR}/examples/smpi/mc ${CMAKE_HOME_DIRECTORY}/examples/smpi/mc/only_send_deterministic.tesh)
    ENDIF()
    # END TESH TESTS
  ENDIF()

  ## BINDINGS ##
  ### LUA ###
  # BEGIN TESH TESTS
  IF(HAVE_LUA)
    # Tests testing simulation from C but using lua for platform files. Executed like this
    # ~$ ./masterslave platform.lua deploy.lua
    ADD_TESH(lua-platform-masterslave                --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/lua --cd ${CMAKE_BINARY_DIR}/examples/lua ${CMAKE_HOME_DIRECTORY}/teshsuite/lua/lua_platforms.tesh)
    SET_TESTS_PROPERTIES(lua-platform-masterslave    PROPERTIES ENVIRONMENT "LUA_CPATH=${CMAKE_BINARY_DIR}/examples/lua/?.so")
  ENDIF()
  # END TESH TESTS

  ### JAVA ###
  IF(enable_java)
    IF(WIN32)
      SET(TESH_CLASSPATH "${CMAKE_BINARY_DIR}/examples/java/\;${CMAKE_BINARY_DIR}/teshsuite/java/\;${SIMGRID_JAR}")
      STRING(REPLACE "\;" "§" TESH_CLASSPATH "${TESH_CLASSPATH}")
    ELSE()
      SET(TESH_CLASSPATH "${CMAKE_BINARY_DIR}/examples/java/:${CMAKE_BINARY_DIR}/teshsuite/java/:${SIMGRID_JAR}")
    ENDIF()
    ADD_TESH(java-async                          --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/async/async.tesh)
    ADD_TESH(java-bittorrent                     --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/bittorrent/bittorrent.tesh)
    ADD_TESH(java-bypass                         --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/master_slave_bypass/bypass.tesh)
    ADD_TESH(java-chord                          --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/chord/chord.tesh)
    ADD_TESH(java-cloud                          --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/cloud/cloud.tesh)
    ADD_TESH(java-cloud-energy                   --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/cloud/energy/energy.tesh)
    ADD_TESH(java-cloud-migration                --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/cloud/migration/migration.tesh)
    ADD_TESH(java-commTime                       --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/commTime/commtime.tesh)
    ADD_TESH(java-energy                         --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/energy/energy.tesh)
    ADD_TESH(java-storage                        --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/io/storage.tesh)
    ADD_TESH(java-kademlia                       --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/kademlia/kademlia.tesh)
    ADD_TESH(java-kill                           --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/master_slave_kill/kill.tesh)
    ADD_TESH(java-masterslave                    --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/masterslave/masterslave.tesh)
    ADD_TESH(java-migration                      --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/migration/migration.tesh)
    ADD_TESH(java-mutualExclusion                --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/mutualExclusion/mutualexclusion.tesh)
    ADD_TESH(java-pingPong                       --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/pingPong/pingpong.tesh)
    ADD_TESH(java-priority                       --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/priority/priority.tesh)
    ADD_TESH(java-startKillTime                  --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/startKillTime/startKillTime.tesh)
    ADD_TESH(java-suspend                        --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/suspend/suspend.tesh)
    ADD_TESH(java-tracing                        --setenv srcdir=${CMAKE_HOME_DIRECTORY}/examples/java --setenv classpath=${TESH_CLASSPATH} --cd ${CMAKE_BINARY_DIR}/examples/java ${CMAKE_HOME_DIRECTORY}/examples/java/tracing/tracingPingPong.tesh)
  ENDIF()
ENDIF()

  ## OTHER ##
ADD_TEST(testall                                 ${CMAKE_BINARY_DIR}/testall)

IF(enable_memcheck)
  INCLUDE(FindValgrind)
  INCLUDE(${CMAKE_HOME_DIRECTORY}/tools/cmake/memcheck_tests.cmake)
ENDIF()
