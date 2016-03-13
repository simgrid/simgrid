IF(enable_smpi AND NOT WIN32)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpicc)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpicxx)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpiff)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpif90)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpirun)
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
      # This test checks if the stack cleaner is making a difference:
      add_test(mc-bugged1-liveness-stack-cleaner
        ${CMAKE_HOME_DIRECTORY}/examples/msg/mc/bugged1_liveness_stack_cleaner
        ${CMAKE_HOME_DIRECTORY}/examples/msg/mc/
        ${CMAKE_BINARY_DIR}/examples/msg/mc/
        )
    endif()
    ENDIF()
  ENDIF()

  ## INTERFACES ##
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

  ## BINDINGS ##
  ### LUA ###
  IF(HAVE_LUA)
    # Tests testing simulation from C but using lua for platform files. Executed like this
    # ~$ ./masterslave platform.lua deploy.lua
    ADD_TESH(lua-platform-masterslave                --setenv srcdir=${CMAKE_HOME_DIRECTORY}/teshsuite/lua --cd ${CMAKE_BINARY_DIR}/examples/lua ${CMAKE_HOME_DIRECTORY}/teshsuite/lua/lua_platforms.tesh)
    SET_TESTS_PROPERTIES(lua-platform-masterslave    PROPERTIES ENVIRONMENT "LUA_CPATH=${CMAKE_BINARY_DIR}/examples/lua/?.so")
  ENDIF()

  ### JAVA ###
  IF(enable_java)
    IF(WIN32)
      SET(TESH_CLASSPATH "${CMAKE_BINARY_DIR}/examples/java/\;${CMAKE_BINARY_DIR}/teshsuite/java/\;${SIMGRID_JAR}")
      STRING(REPLACE "\;" "§" TESH_CLASSPATH "${TESH_CLASSPATH}")
    ELSE()
      SET(TESH_CLASSPATH "${CMAKE_BINARY_DIR}/examples/java/:${CMAKE_BINARY_DIR}/teshsuite/java/:${SIMGRID_JAR}")
    ENDIF()
  ENDIF()
ENDIF()

  ## OTHER ##
ADD_TEST(testall                                 ${CMAKE_BINARY_DIR}/testall)

IF(enable_memcheck)
  INCLUDE(FindValgrind)
  INCLUDE(${CMAKE_BINARY_DIR}/memcheck_tests.cmake)
ENDIF()
