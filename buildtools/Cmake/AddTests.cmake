### Be sure we can execut some launch file
exec_program("chmod a=rwx ${PROJECT_DIRECTORY}/buildtools/Cmake/test_java.sh" OUTPUT_VARIABLE "OKITOKI")
if(enable_smpi)
	exec_program("chmod a=rwx ${PROJECT_DIRECTORY}/src/smpi/smpicc" OUTPUT_VARIABLE "OKITOKI")
	exec_program("chmod a=rwx ${PROJECT_DIRECTORY}/src/smpi/smpirun" OUTPUT_VARIABLE "OKITOKI")
endif(enable_smpi)

if(enable_memcheck)
	include(FindPerl)
	exec_program("valgrind --version " OUTPUT_VARIABLE "VALGRIND_VERSION")
	if(VALGRIND_VERSION AND PERL_EXECUTABLE)
		string(REGEX MATCH "[0-9].[0-9].[0-9]" NEW_VALGRIND_VERSION "${VALGRIND_VERSION}")
		if(NEW_VALGRIND_VERSION)
			exec_program("${PROJECT_DIRECTORY}/buildtools/Cmake/generate_memcheck_tests.pl ${PROJECT_DIRECTORY} ${PROJECT_DIRECTORY}/buildtools/Cmake/AddTests.cmake > ${PROJECT_DIRECTORY}/buildtools/Cmake/memcheck_tests.cmake")
		else(NEW_VALGRIND_VERSION)
			set(enable_memcheck false)
			message("Command valgrind not found --> enable_memcheck autoset to false.")
		endif(NEW_VALGRIND_VERSION)
	else(VALGRIND_VERSION AND PERL_EXECUTABLE)
		set(enable_memcheck false)
		message(FATAL_ERROR "Command valgrind or perl not found --> enable_memcheck autoset to false.")
	endif(VALGRIND_VERSION AND PERL_EXECUTABLE)
endif(enable_memcheck)

### For code coverage
### Set some variables
SET(UPDATE_TYPE "svn")
SET(DROP_METHOD "http")
SET(DROP_SITE "cdash.inria.fr/CDash")
SET(DROP_LOCATION "/submit.php?project=${PROJECT_NAME}")
SET(DROP_SITE_CDASH TRUE)
SET(TRIGGER_SITE "http://cdash.inria.fr/CDash/cgi-bin/Submit-Random-TestingResults.cgi")
SET(COVERAGE_COMMAND "${GCOV_PATH}/gcov")
set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full --show-reachable=yes --track-origins=yes --read-var-info=no")
SET(VALGRIND_COMMAND "${PROJECT_DIRECTORY}/buildtools/Cmake/my_valgrind.pl")
SET(MEMORYCHECK_COMMAND "${PROJECT_DIRECTORY}/buildtools/Cmake/my_valgrind.pl")
#If you use the --read-var-info option Memcheck will run more slowly but may give a more detailed description of any illegal address.

INCLUDE(CTest)
ENABLE_TESTING()

if(NOT enable_memcheck)
ADD_TEST(tesh-self-basic 		${CMAKE_BINARY_DIR}/bin/tesh --cd "${PROJECT_DIRECTORY}/tools/tesh" basic.tesh)
ADD_TEST(tesh-self-cd			${CMAKE_BINARY_DIR}/bin/tesh --cd "${CMAKE_BINARY_DIR}/bin" ${PROJECT_DIRECTORY}/tools/tesh/cd.tesh)
ADD_TEST(tesh-self-IO-broken-pipe	${CMAKE_BINARY_DIR}/bin/tesh --cd "${PROJECT_DIRECTORY}/tools/tesh" IO-broken-pipe.tesh)
ADD_TEST(tesh-self-IO-orders		${CMAKE_BINARY_DIR}/bin/tesh --cd "${CMAKE_BINARY_DIR}/bin" ${PROJECT_DIRECTORY}/tools/tesh/IO-orders.tesh)
ADD_TEST(tesh-self-IO-bigsize		${CMAKE_BINARY_DIR}/bin/tesh --cd "${PROJECT_DIRECTORY}/tools/tesh" IO-bigsize.tesh)
ADD_TEST(tesh-self-set-return		${CMAKE_BINARY_DIR}/bin/tesh --cd "${PROJECT_DIRECTORY}/tools/tesh" set-return.tesh)
ADD_TEST(tesh-self-set-signal		${CMAKE_BINARY_DIR}/bin/tesh --cd "${PROJECT_DIRECTORY}/tools/tesh" set-signal.tesh)
ADD_TEST(tesh-self-set-timeout		${CMAKE_BINARY_DIR}/bin/tesh --cd "${PROJECT_DIRECTORY}/tools/tesh" set-timeout.tesh)
ADD_TEST(tesh-self-set-ignore-output	${CMAKE_BINARY_DIR}/bin/tesh --cd "${CMAKE_BINARY_DIR}/bin" ${PROJECT_DIRECTORY}/tools/tesh/set-ignore-output.tesh)
ADD_TEST(tesh-self-catch-return		${CMAKE_BINARY_DIR}/bin/tesh --cd "${CMAKE_BINARY_DIR}/bin" ${PROJECT_DIRECTORY}/tools/tesh/catch-return.tesh)
ADD_TEST(tesh-self-catch-signal		${CMAKE_BINARY_DIR}/bin/tesh --cd "${CMAKE_BINARY_DIR}/bin" ${PROJECT_DIRECTORY}/tools/tesh/catch-signal.tesh)
ADD_TEST(tesh-self-catch-timeout	${CMAKE_BINARY_DIR}/bin/tesh --cd "${CMAKE_BINARY_DIR}/bin" ${PROJECT_DIRECTORY}/tools/tesh/catch-timeout.tesh)
ADD_TEST(tesh-self-catch-wrong-output	${CMAKE_BINARY_DIR}/bin/tesh --cd "${CMAKE_BINARY_DIR}/bin" ${PROJECT_DIRECTORY}/tools/tesh/catch-wrong-output.tesh)
ADD_TEST(tesh-self-bg-basic		${CMAKE_BINARY_DIR}/bin/tesh --cd "${PROJECT_DIRECTORY}/tools/tesh" bg-basic.tesh)
ADD_TEST(tesh-self-bg-set-signal	${CMAKE_BINARY_DIR}/bin/tesh --cd "${PROJECT_DIRECTORY}/tools/tesh" bg-set-signal.tesh)
ADD_TEST(tesh-self-background		${CMAKE_BINARY_DIR}/bin/tesh --cd "${PROJECT_DIRECTORY}/tools/tesh" background.tesh)

# BEGIN TESH TESTS

# teshsuite/xbt
ADD_TEST(tesh-log-large		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite xbt/log_large_test.tesh)
ADD_TEST(tesh-log-parallel	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite xbt/parallel_log_crashtest.tesh)

# teshsuite/gras/datadesc directory
ADD_TEST(tesh-gras-dd-mem	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite gras/datadesc/datadesc_mem.tesh)
ADD_TEST(tesh-gras-dd-rw 	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite gras/datadesc/datadesc_rw.tesh)
ADD_TEST(tesh-gras-dd-r_little32_4	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite gras/datadesc/datadesc_r_little32_4.tesh)
ADD_TEST(tesh-gras-dd-r_little64	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite gras/datadesc/datadesc_r_little64.tesh)
ADD_TEST(tesh-gras-dd-r_big32_8_4	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite gras/datadesc/datadesc_r_big32_8_4.tesh)
ADD_TEST(tesh-gras-dd-r_big32_8		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite gras/datadesc/datadesc_r_big32_8.tesh)
ADD_TEST(tesh-gras-dd-r_big32_2		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite gras/datadesc/datadesc_r_big32_2.tesh)

IF(${ARCH_32_BITS})
  ADD_TEST(tesh-gras-msg_handle-sg-32	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite gras/msg_handle/test_sg_32)
ELSE(${ARCH_32_BITS})
  ADD_TEST(tesh-gras-msg_handle-sg-64	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite gras/msg_handle/test_sg_64)
ENDIF(${ARCH_32_BITS})

ADD_TEST(tesh-gras-empty_main-rl	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite gras/empty_main/test_rl)
ADD_TEST(tesh-gras-empty_main-sg	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite gras/empty_main/test_sg)

IF(${ARCH_32_BITS})
  ADD_TEST(tesh-gras-small_sleep-sg-32	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite gras/small_sleep/test_sg_32)
ELSE(${ARCH_32_BITS})
  ADD_TEST(tesh-gras-small_sleep-sg-64	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite gras/small_sleep/test_sg_64)
ENDIF(${ARCH_32_BITS})

ADD_TEST(tesh-msg-get_sender	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite msg/get_sender.tesh)				    
ADD_TEST(tesh-simdag-reinit_costs	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite simdag/network/test_reinit_costs.tesh)
ADD_TEST(tesh-simdag-parser		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite simdag/platforms/basic_parsing_test.tesh)
ADD_TEST(tesh-simdag-flatifier		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite simdag/platforms/flatifier.tesh)
ADD_TEST(tesh-simdag-basic0	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite simdag/basic0.tesh)
ADD_TEST(tesh-simdag-basic1	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite simdag/basic1.tesh)
ADD_TEST(tesh-simdag-basic2	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite simdag/basic2.tesh)
ADD_TEST(tesh-simdag-basic3	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite simdag/basic3.tesh)
ADD_TEST(tesh-simdag-basic4	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite simdag/basic4.tesh)
ADD_TEST(tesh-simdag-basic5	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite simdag/basic5.tesh)
ADD_TEST(tesh-simdag-basic6	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite simdag/basic6.tesh)
ADD_TEST(tesh-simdag-p2p-1	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite simdag/network/p2p/test_latency1.tesh)
ADD_TEST(tesh-simdag-p2p-2	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite simdag/network/p2p/test_latency2.tesh)
ADD_TEST(tesh-simdag-p2p-3	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite simdag/network/p2p/test_latency3.tesh)
ADD_TEST(tesh-simdag-p2p-3	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite simdag/network/p2p/test_latency_bound.tesh)
ADD_TEST(tesh-simdag-mxn-1	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite simdag/network/mxn/test_intra_all2all.tesh)
ADD_TEST(tesh-simdag-mxn-2	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite simdag/network/mxn/test_intra_independent_comm.tesh)
ADD_TEST(tesh-simdag-mxn-3	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite simdag/network/mxn/test_intra_scatter.tesh)
ADD_TEST(tesh-simdag-par-1	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite simdag/partask/test_comp_only_seq.tesh)
ADD_TEST(tesh-simdag-par-2	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/teshsuite simdag/partask/test_comp_only_par.tesh)

# GRAS examples
ADD_TEST(gras-ping-rl		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/ping test_rl)
ADD_TEST(gras-rpc-rl		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/rpc test_rl)
ADD_TEST(gras-spawn-rl		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/spawn test_rl)
ADD_TEST(gras-timer-rl		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/timer test_rl)
ADD_TEST(gras-chrono-rl		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/chrono test_rl)
ADD_TEST(gras-simple_token-rl	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/mutual_exclusion/simple_token test_rl)
ADD_TEST(gras-mmrpc-rl		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/mmrpc test_rl)
ADD_TEST(gras-all2all-rl	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/all2all test_rl)
ADD_TEST(gras-pmm-rl		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/pmm test_rl)
ADD_TEST(gras-synchro-rl	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/synchro test_rl)
ADD_TEST(gras-properties-rl	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/properties test_rl)

# MSG examples
ADD_TEST(msg-sendrecv_CLM03	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg sendrecv/sendrecv_CLM03.tesh)
ADD_TEST(msg-sendrecv_Vegas	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg sendrecv/sendrecv_Vegas.tesh)
ADD_TEST(msg-sendrecv_Reno	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg sendrecv/sendrecv_Reno.tesh)
ADD_TEST(msg-suspend		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg suspend/suspend.tesh)
ADD_TEST(msg-masterslave	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg masterslave/masterslave.tesh)
ADD_TEST(msg-masterslave-forwarder	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg masterslave/masterslave_forwarder.tesh)
ADD_TEST(msg-masterslave-failure	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg masterslave/masterslave_failure.tesh)
ADD_TEST(msg-masterslave-bypass	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg masterslave/masterslave_bypass.tesh)
ADD_TEST(msg-masterslave-mailbox	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg masterslave/masterslave_mailbox.tesh)
ADD_TEST(msg-masterslave-vivaldi	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg masterslave/masterslave_vivaldi.tesh)
ADD_TEST(msg-migration		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg migration/migration.tesh)
ADD_TEST(msg-ptask		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg parallel_task/parallel_task.tesh)
ADD_TEST(msg-priority		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg priority/priority.tesh)
ADD_TEST(msg-properties		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg properties/msg_prop.tesh)
ADD_TEST(msg-trace		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg trace/trace.tesh)
ADD_TEST(msg-masterslave_cpu_ti	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg masterslave/masterslave_cpu_ti.tesh)

IF(HAVE_TRACING)
  ADD_TEST(tracing-ms ${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg tracing/ms.tesh)
  ADD_TEST(tracing-categories ${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg tracing/categories.tesh)
  ADD_TEST(tracing-volume ${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg tracing/volume.tesh)
  ADD_TEST(tracing-tasks ${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg tracing/tasks.tesh)
  ADD_TEST(tracing-process-migration ${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg tracing/procmig.tesh)
ENDIF(HAVE_TRACING)

IF(${ARCH_32_BITS})
  ADD_TEST(gras-ping-sg-32	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/ping test_sg_32)
  ADD_TEST(gras-rpc-sg-32	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/rpc test_sg_32)
  ADD_TEST(gras-spawn-sg-32	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/spawn test_sg_32)
  ADD_TEST(gras-timer-sg-32	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/timer test_sg_32)
  ADD_TEST(gras-chrono-sg-32	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/chrono test_sg_32)
  ADD_TEST(gras-simple_token-sg-32	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/mutual_exclusion/simple_token test_sg_32)
  ADD_TEST(gras-mmrpc-sg-32	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/mmrpc test_sg_32)
  ADD_TEST(gras-all2all-sg-32	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/all2all test_sg_32)
  ADD_TEST(gras-pmm-sg-32	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/pmm test_sg_32)
  ADD_TEST(gras-synchro-sg-32	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/synchro test_sg_32)
ELSE(${ARCH_32_BITS})
  ADD_TEST(gras-ping-sg-64	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/ping test_sg_64)
  ADD_TEST(gras-rpc-sg-64	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/rpc test_sg_64)
  ADD_TEST(gras-spawn-sg-64	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/spawn test_sg_64)
  ADD_TEST(gras-timer-sg-64	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/timer test_sg_64)
  ADD_TEST(gras-chrono-sg-64	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/chrono test_sg_64)
  ADD_TEST(gras-simple_token-sg-64	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/mutual_exclusion/simple_token test_sg_64)
  ADD_TEST(gras-mmrpc-sg-64	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/mmrpc test_sg_64)
  ADD_TEST(gras-all2all-sg-64	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/all2all test_sg_64)
  ADD_TEST(gras-pmm-sg-64	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/pmm test_sg_64)
  ADD_TEST(gras-synchro-sg-64	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/synchro test_sg_64)
ENDIF(${ARCH_32_BITS})
ADD_TEST(gras-properties-sg	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/gras/properties test_sg)

# amok examples
ADD_TEST(amok-bandwidth-rl	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/amok bandwidth/bandwidth_rl.tesh)
ADD_TEST(amok-saturate-rl	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/amok saturate/saturate_rl.tesh)
IF(${ARCH_32_BITS})
  ADD_TEST(amok-bandwidth-sg-32	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/amok bandwidth/bandwidth_sg_32.tesh)
  ADD_TEST(amok-saturate-sg-32	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/amok saturate/saturate_sg_32.tesh)
ELSE(${ARCH_32_BITS})
  ADD_TEST(amok-bandwidth-sg-64	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/amok bandwidth/bandwidth_sg_64.tesh)
  ADD_TEST(amok-saturate-sg-64	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/amok saturate/saturate_sg_64.tesh)
ENDIF(${ARCH_32_BITS})

# simdag examples
ADD_TEST(simdag-test_simdag	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/simdag test_simdag.tesh)
ADD_TEST(simdag-test_simdag2	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/simdag test_simdag2.tesh)
ADD_TEST(simdag-test_simdag_seq_access	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/simdag test_simdag_seq_access.tesh)
ADD_TEST(simdag-test_prop	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/simdag properties/test_prop.tesh)
ADD_TEST(simdag-metaxml_test	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/simdag metaxml/metaxml_test.tesh)
ADD_TEST(simdag-minmin_test	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/simdag/scheduling test_minmin.tesh)

if(enable_smpi)
# smpi examples
ADD_TEST(smpi-bcast 	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/smpi bcast.tesh)
ADD_TEST(smpi-reduce 	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/smpi reduce.tesh)
endif(enable_smpi)

if(HAVE_GTNETS)
ADD_TEST(msg-gtnets-waxman	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg gtnets/gtnets-waxman.tesh)
ADD_TEST(msg-gtnets-dogbone	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg gtnets/gtnets-dogbone-gtnets.tesh)
ADD_TEST(msg-gtnets-onelink	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg gtnets/gtnets-onelink-gtnets.tesh)
ADD_TEST(msg-gtnets-dogbone-lv08	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg gtnets/gtnets-dogbone-lv08.tesh)
ADD_TEST(msg-gtnets-onelink-lv08	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg gtnets/gtnets-onelink-lv08.tesh)
endif(HAVE_GTNETS)

# Lua examples
if(HAVE_LUA)
ADD_TEST(lua-masterslave	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/lua master_slave.tesh)
ADD_TEST(lua-mult_matrix	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/lua mult_matrix.tesh)
endif(HAVE_LUA)

# Ruby examples
if(HAVE_RUBY)
ADD_TEST(ruby-masterslave	${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/ruby MasterSlave.tesh)
ADD_TEST(ruby-ping_pong		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/ruby PingPong.tesh)
ADD_TEST(ruby-quicksort		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/ruby Quicksort.tesh)
endif(HAVE_RUBY)

# END TESH TESTS

if(HAVE_MC)
ADD_TEST(mc-bugged1			${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg/mc bugged1.tesh)
ADD_TEST(mc-bugged2			${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg/mc bugged2.tesh)
ADD_TEST(mc-centralized		${CMAKE_BINARY_DIR}/bin/tesh --cd ${PROJECT_DIRECTORY}/examples/msg/mc centralized.tesh)
endif(HAVE_MC)

if(HAVE_JAVA)
# java examples
ADD_TEST(java-basic 	${PROJECT_DIRECTORY}/buildtools/Cmake/test_java.sh ${PROJECT_DIRECTORY}/examples/java/basic BasicTest ${PROJECT_DIRECTORY})
ADD_TEST(java-pingpong 	${PROJECT_DIRECTORY}/buildtools/Cmake/test_java.sh ${PROJECT_DIRECTORY}/examples/java/ping_pong PingPongTest ${PROJECT_DIRECTORY})
ADD_TEST(java-comm_time ${PROJECT_DIRECTORY}/buildtools/Cmake/test_java.sh ${PROJECT_DIRECTORY}/examples/java/comm_time CommTimeTest ${PROJECT_DIRECTORY})
ADD_TEST(java-suspend 	${PROJECT_DIRECTORY}/buildtools/Cmake/test_java.sh ${PROJECT_DIRECTORY}/examples/java/suspend SuspendTest ${PROJECT_DIRECTORY})
endif(HAVE_JAVA)

###
### Declare that we know that some tests are broken
###

# Amok is broken in RL since before v3.3 (should fix it one day)
set_tests_properties(amok-bandwidth-rl PROPERTIES WILL_FAIL true)
set_tests_properties(amok-saturate-rl PROPERTIES WILL_FAIL true)

# Expected to fail until we regenerate the data
set_tests_properties(tesh-gras-dd-r_big32_8 PROPERTIES WILL_FAIL true)
set_tests_properties(tesh-gras-dd-r_big32_2 PROPERTIES WILL_FAIL true)

# Expected to fail until the parser gets better (v3.3.5?)
set_tests_properties(simdag-metaxml_test PROPERTIES WILL_FAIL true)   
set_tests_properties(tesh-simdag-flatifier PROPERTIES WILL_FAIL true)

if(HAVE_RUBY)
set_tests_properties(ruby-quicksort PROPERTIES WILL_FAIL true)
endif(HAVE_RUBY)
endif(NOT enable_memcheck)

# testsuite directory
add_test(test-xbt-log		${PROJECT_DIRECTORY}/testsuite/xbt/log_usage)
add_test(test-xbt-graphxml	${PROJECT_DIRECTORY}/testsuite/xbt/graphxml_usage ${PROJECT_DIRECTORY}/testsuite/xbt/graph.xml)
add_test(test-xbt-heap		${PROJECT_DIRECTORY}/testsuite/xbt/heap_bench)

add_test(test-surf-lmm		${PROJECT_DIRECTORY}/testsuite/surf/lmm_usage)
add_test(test-surf-maxmin	${PROJECT_DIRECTORY}/testsuite/surf/maxmin_bench)
add_test(test-surf-usage	${PROJECT_DIRECTORY}/testsuite/surf/surf_usage  --cfg=path:${PROJECT_DIRECTORY}/testsuite/surf/ platform.xml)
add_test(test-surf_usage2	${PROJECT_DIRECTORY}/testsuite/surf/surf_usage2  --cfg=path:${PROJECT_DIRECTORY}/testsuite/surf/ platform.xml)
add_test(test-surf-trace	${PROJECT_DIRECTORY}/testsuite/surf/trace_usage --cfg=path:${PROJECT_DIRECTORY}/testsuite/surf/)

add_test(test-simdag-1 ${PROJECT_DIRECTORY}/testsuite/simdag/sd_test --cfg=path:${PROJECT_DIRECTORY}/testsuite/simdag small_platform_variable.xml)
add_test(test-simdag-2 ${PROJECT_DIRECTORY}/testsuite/simdag/sd_test --cfg=path:${PROJECT_DIRECTORY}/testsuite/simdag ${PROJECT_DIRECTORY}/examples/msg/small_platform.xml)
add_test(test-simdag-3 ${PROJECT_DIRECTORY}/testsuite/simdag/sd_test --cfg=path:${PROJECT_DIRECTORY}/testsuite/simdag ${PROJECT_DIRECTORY}/examples/msg/msg_platform.xml)

add_test(testall		${CMAKE_BUILD_DIR}/src/testall)

include(${PROJECT_DIRECTORY}/buildtools/Cmake/memcheck_tests.cmake)
