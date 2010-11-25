if(enable_memcheck)
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
set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full --show-reachable=yes --track-origins=yes --read-var-info=no")
SET(VALGRIND_COMMAND "${PROJECT_DIRECTORY}/buildtools/Cmake/my_valgrind.pl")
SET(MEMORYCHECK_COMMAND "${PROJECT_DIRECTORY}/buildtools/Cmake/my_valgrind.pl")
#If you use the --read-var-info option Memcheck will run more slowly but may give a more detailed description of any illegal address.

INCLUDE(CTest)
ENABLE_TESTING()

# BEGIN TESH TESTS

# teshsuite/xbt
IF(${ARCH_32_BITS})
  ADD_TEST(tesh-gras-msg_handle-sg-32	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite gras/msg_handle/test_sg_32)
ELSE(${ARCH_32_BITS})
  ADD_TEST(tesh-gras-msg_handle-sg-64	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite gras/msg_handle/test_sg_64)
ENDIF(${ARCH_32_BITS})

ADD_TEST(tesh-gras-empty_main-rl	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite gras/empty_main/test_rl)
ADD_TEST(tesh-gras-empty_main-sg	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite gras/empty_main/test_sg)

IF(${ARCH_32_BITS})
  ADD_TEST(tesh-gras-small_sleep-sg-32	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite gras/small_sleep/test_sg_32)
ELSE(${ARCH_32_BITS})
  ADD_TEST(tesh-gras-small_sleep-sg-64	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite gras/small_sleep/test_sg_64)
ENDIF(${ARCH_32_BITS})

ADD_TEST(tesh-msg-get_sender	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite msg/get_sender.tesh)				    
ADD_TEST(tesh-simdag-reinit_costs	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite simdag/network/test_reinit_costs.tesh)
ADD_TEST(tesh-simdag-parser		perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite/simdag/platforms basic_parsing_test.tesh)
ADD_TEST(tesh-simdag-flatifier		perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite/simdag/platforms flatifier.tesh)
ADD_TEST(tesh-simdag-basic0	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite simdag/basic0.tesh)
ADD_TEST(tesh-simdag-basic1	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite simdag/basic1.tesh)
ADD_TEST(tesh-simdag-basic2	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite simdag/basic2.tesh)
ADD_TEST(tesh-simdag-basic3	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite simdag/basic3.tesh)
ADD_TEST(tesh-simdag-basic4	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite simdag/basic4.tesh)
ADD_TEST(tesh-simdag-basic5	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite simdag/basic5.tesh)
ADD_TEST(tesh-simdag-basic6	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite simdag/basic6.tesh)
ADD_TEST(tesh-simdag-p2p-1	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite simdag/network/p2p/test_latency1.tesh)
ADD_TEST(tesh-simdag-p2p-2	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite simdag/network/p2p/test_latency2.tesh)
ADD_TEST(tesh-simdag-p2p-3	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite simdag/network/p2p/test_latency3.tesh)
ADD_TEST(tesh-simdag-p2p-3	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite simdag/network/p2p/test_latency_bound.tesh)
ADD_TEST(tesh-simdag-mxn-1	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite simdag/network/mxn/test_intra_all2all.tesh)
ADD_TEST(tesh-simdag-mxn-2	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite simdag/network/mxn/test_intra_independent_comm.tesh)
ADD_TEST(tesh-simdag-mxn-3	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite simdag/network/mxn/test_intra_scatter.tesh)
ADD_TEST(tesh-simdag-par-1	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite simdag/partask/test_comp_only_seq.tesh)
ADD_TEST(tesh-simdag-par-2	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/teshsuite simdag/partask/test_comp_only_par.tesh)

# GRAS examples
ADD_TEST(gras-ping-rl		perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/ping test_rl)
ADD_TEST(gras-rpc-rl		perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/rpc test_rl)
ADD_TEST(gras-spawn-rl		perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/spawn test_rl)

ADD_TEST(gras-timer-rl		perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/timer test_rl)
set_tests_properties(gras-timer-rl PROPERTIES TIMEOUT 10)

ADD_TEST(gras-chrono-rl		perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/chrono test_rl)
set_tests_properties(gras-chrono-rl PROPERTIES TIMEOUT 10)

ADD_TEST(gras-simple_token-rl	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/mutual_exclusion/simple_token test_rl)

ADD_TEST(gras-mmrpc-rl		perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/mmrpc test_rl)
set_tests_properties(gras-mmrpc-rl PROPERTIES TIMEOUT 10)

ADD_TEST(gras-all2all-rl	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/all2all test_rl)
set_tests_properties(gras-all2all-rl PROPERTIES TIMEOUT 10)

ADD_TEST(gras-pmm-rl		perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/pmm test_rl)
set_tests_properties(gras-pmm-rl PROPERTIES TIMEOUT 10)

ADD_TEST(gras-synchro-rl	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/synchro test_rl)
set_tests_properties(gras-synchro-rl PROPERTIES TIMEOUT 10)

ADD_TEST(gras-properties-rl	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/properties test_rl)
set_tests_properties(gras-properties-rl PROPERTIES TIMEOUT 10)

# MSG examples
ADD_TEST(msg-suspend		perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg suspend/suspend.tesh)
ADD_TEST(msg-masterslave	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg masterslave/masterslave.tesh)
ADD_TEST(msg-masterslave-forwarder	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg masterslave/masterslave_forwarder.tesh)
ADD_TEST(msg-masterslave-failure	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg masterslave/masterslave_failure.tesh)
ADD_TEST(msg-masterslave-bypass	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg masterslave/masterslave_bypass.tesh)
ADD_TEST(msg-masterslave-mailbox	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg masterslave/masterslave_mailbox.tesh)
ADD_TEST(msg-masterslave-vivaldi	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg masterslave/masterslave_vivaldi.tesh)
ADD_TEST(msg-migration		perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg migration/migration.tesh)
ADD_TEST(msg-ptask		perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg parallel_task/parallel_task.tesh)
ADD_TEST(msg-priority		${PROJECT_DIRECTORY}/examples/msg/priority/priority.exe ${PROJECT_DIRECTORY}/examples/msg/small_platform.xml  ${PROJECT_DIRECTORY}/examples/msg/priority/deployment_priority.xml)
ADD_TEST(msg-properties		perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg properties/msg_prop.tesh)
ADD_TEST(msg-trace		perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg trace/trace.tesh)
ADD_TEST(msg-masterslave_cpu_ti	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg masterslave/masterslave_cpu_ti.tesh)
ADD_TEST(msg_icomms perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg/icomms peer.tesh)
ADD_TEST(msg_icomms_waitany ${PROJECT_DIRECTORY}/examples/msg/icomms/peer3 
								${PROJECT_DIRECTORY}/examples/msg/icomms/small_platform.xml
								${PROJECT_DIRECTORY}/examples/msg/icomms/deployment_peer05.xml)

IF(HAVE_TRACING)
  ADD_TEST(tracing-ms perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg tracing/ms.tesh)
  ADD_TEST(tracing-categories perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg tracing/categories.tesh)
  ADD_TEST(tracing-volume perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg tracing/volume.tesh)
  ADD_TEST(tracing-tasks perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg tracing/tasks.tesh)
  ADD_TEST(tracing-process-migration perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg tracing/procmig.tesh)
ENDIF(HAVE_TRACING)

IF(${ARCH_32_BITS})
  ADD_TEST(gras-ping-sg-32	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/ping test_sg_32)
  ADD_TEST(gras-rpc-sg-32	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/rpc test_sg_32)
  ADD_TEST(gras-spawn-sg-32	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/spawn test_sg_32)
  ADD_TEST(gras-timer-sg-32	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/timer test_sg_32)
  ADD_TEST(gras-chrono-sg-32	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/chrono test_sg_32)
  ADD_TEST(gras-simple_token-sg-32	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/mutual_exclusion/simple_token test_sg_32)
  ADD_TEST(gras-mmrpc-sg-32	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/mmrpc test_sg_32)
  ADD_TEST(gras-all2all-sg-32	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/all2all test_sg_32)
  ADD_TEST(gras-pmm-sg-32	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/pmm test_sg_32)
  ADD_TEST(gras-synchro-sg-32	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/synchro test_sg_32)
ELSE(${ARCH_32_BITS})
  ADD_TEST(gras-ping-sg-64	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/ping test_sg_64)
  ADD_TEST(gras-rpc-sg-64	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/rpc test_sg_64)
  ADD_TEST(gras-spawn-sg-64	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/spawn test_sg_64)
  ADD_TEST(gras-timer-sg-64	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/timer test_sg_64)
  ADD_TEST(gras-chrono-sg-64	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/chrono test_sg_64)
  ADD_TEST(gras-simple_token-sg-64	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/mutual_exclusion/simple_token test_sg_64)
  ADD_TEST(gras-mmrpc-sg-64	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/mmrpc test_sg_64)
  ADD_TEST(gras-all2all-sg-64	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/all2all test_sg_64)
  ADD_TEST(gras-pmm-sg-64	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/pmm test_sg_64)
  ADD_TEST(gras-synchro-sg-64	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/synchro test_sg_64)
ENDIF(${ARCH_32_BITS})
ADD_TEST(gras-properties-sg	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/gras/properties test_sg)

# simdag examples
ADD_TEST(simdag-test_simdag	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/simdag test_simdag.tesh)
ADD_TEST(simdag-test_simdag2	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/simdag test_simdag2.tesh)
ADD_TEST(simdag-test_simdag_seq_access	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/simdag test_simdag_seq_access.tesh)
ADD_TEST(simdag-test_prop	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/simdag properties/test_prop.tesh)
ADD_TEST(simdag-metaxml_test	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/simdag metaxml/metaxml_test.tesh)
set_tests_properties(simdag-metaxml_test PROPERTIES WILL_FAIL true)
ADD_TEST(simdag-minmin_test	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/simdag/scheduling test_minmin.tesh)

if(enable_smpi)
# smpi examples
ADD_TEST(smpi-bcast 	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${CMAKE_BINARY_DIR}/examples/smpi ${PROJECT_DIRECTORY}/examples/smpi/bcast.tesh)
ADD_TEST(smpi-reduce 	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${CMAKE_BINARY_DIR}/examples/smpi ${PROJECT_DIRECTORY}/examples/smpi/reduce.tesh)
if(HAVE_TRACING)
  ADD_TEST(smpi-tracing-ptp perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${CMAKE_BINARY_DIR}/examples/smpi ${PROJECT_DIRECTORY}/examples/smpi/smpi_traced.tesh)
endif(HAVE_TRACING)
endif(enable_smpi)

if(HAVE_GTNETS)
ADD_TEST(msg-gtnets-waxman			perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg gtnets/gtnets-waxman.tesh)
ADD_TEST(msg-gtnets-dogbone			perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg gtnets/gtnets-dogbone-gtnets.tesh)
ADD_TEST(msg-gtnets-onelink			perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg gtnets/gtnets-onelink-gtnets.tesh)
ADD_TEST(msg-gtnets-dogbone-lv08	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg gtnets/gtnets-dogbone-lv08.tesh)
ADD_TEST(msg-gtnets-onelink-lv08	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg gtnets/gtnets-onelink-lv08.tesh)
  if(HAVE_TRACING)
  ADD_TEST(msg-tracing-gtnets-waxman			perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg gtnets/tracing-gtnets-waxman.tesh)
  ADD_TEST(msg-tracing-gtnets-dogbone			perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg gtnets/tracing-gtnets-dogbone-gtnets.tesh)
  ADD_TEST(msg-tracing-gtnets-onelink			perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg gtnets/tracing-gtnets-onelink-gtnets.tesh)
  ADD_TEST(msg-tracing-gtnets-dogbone-lv08	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg gtnets/tracing-gtnets-dogbone-lv08.tesh)
  ADD_TEST(msg-tracing-gtnets-onelink-lv08	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg gtnets/tracing-gtnets-onelink-lv08.tesh)
  endif(HAVE_TRACING)
endif(HAVE_GTNETS)

# Lua examples
if(HAVE_LUA)
ADD_TEST(lua-masterslave		perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/lua master_slave.tesh)
ADD_TEST(lua-mult_matrix		perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/lua mult_matrix.tesh)
ADD_TEST(lua-masterslave_bypass perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/lua master_slave_bypass.tesh)
ADD_TEST(msg-masterslave-console	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg/masterslave/ masterslave_console.tesh)
endif(HAVE_LUA)

# Ruby examples
if(HAVE_RUBY)
ADD_TEST(ruby-masterslave	perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/ruby MasterSlave.tesh)
ADD_TEST(ruby-ping_pong		perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/ruby PingPong.tesh)
ADD_TEST(ruby-quicksort		perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/ruby Quicksort.tesh)
endif(HAVE_RUBY)

# END TESH TESTS

if(HAVE_MC)
ADD_TEST(mc-bugged1			perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg/mc bugged1.tesh)
ADD_TEST(mc-bugged2			perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg/mc bugged2.tesh)
ADD_TEST(mc-centralized		perl ${PROJECT_DIRECTORY}/buildtools/Cmake/tesh.pl ${PROJECT_DIRECTORY}/examples/msg/mc centralized.tesh)
endif(HAVE_MC)

if(HAVE_JAVA)
# java examples
ADD_TEST(java-basic 	${PROJECT_DIRECTORY}/buildtools/Cmake/test_java.sh ${PROJECT_DIRECTORY}/examples/java/basic BasicTest ${simgrid_BINARY_DIR})
ADD_TEST(java-pingpong 	${PROJECT_DIRECTORY}/buildtools/Cmake/test_java.sh ${PROJECT_DIRECTORY}/examples/java/ping_pong PingPongTest ${simgrid_BINARY_DIR})
ADD_TEST(java-comm_time ${PROJECT_DIRECTORY}/buildtools/Cmake/test_java.sh ${PROJECT_DIRECTORY}/examples/java/comm_time CommTimeTest ${simgrid_BINARY_DIR})
ADD_TEST(java-suspend 	${PROJECT_DIRECTORY}/buildtools/Cmake/test_java.sh ${PROJECT_DIRECTORY}/examples/java/suspend SuspendTest ${simgrid_BINARY_DIR})
endif(HAVE_JAVA)

if(HAVE_RUBY)
set_tests_properties(ruby-quicksort PROPERTIES WILL_FAIL true)
endif(HAVE_RUBY)

ADD_TEST(tesh-log-large		${PROJECT_DIRECTORY}/teshsuite/xbt/log_large_test --log=root.fmt:%m%n)

ADD_TEST(msg-sendrecv_CLM03	${PROJECT_DIRECTORY}/examples/msg/sendrecv/sendrecv.exe ${PROJECT_DIRECTORY}/examples/msg/sendrecv/platform_sendrecv.xml ${PROJECT_DIRECTORY}/examples/msg/sendrecv/deployment_sendrecv.xml --cfg=workstation/model:CLM03 --cfg=cpu/model:Cas01 --cfg=network/model:CM02)
ADD_TEST(msg-sendrecv_Vegas	${PROJECT_DIRECTORY}/examples/msg/sendrecv/sendrecv.exe ${PROJECT_DIRECTORY}/examples/msg/sendrecv/platform_sendrecv.xml ${PROJECT_DIRECTORY}/examples/msg/sendrecv/deployment_sendrecv.xml "--cfg=workstation/model:compound cpu/model:Cas01 network/model:Vegas")
ADD_TEST(msg-sendrecv_Reno	${PROJECT_DIRECTORY}/examples/msg/sendrecv/sendrecv.exe ${PROJECT_DIRECTORY}/examples/msg/sendrecv/platform_sendrecv.xml ${PROJECT_DIRECTORY}/examples/msg/sendrecv/deployment_sendrecv.xml "--cfg=workstation/model:compound cpu/model:Cas01 network/model:Reno" --log=surf_lagrange.thres=critical)


# testsuite directory
add_test(test-xbt-log		${PROJECT_DIRECTORY}/testsuite/xbt/log_usage)
add_test(test-xbt-graphxml	${PROJECT_DIRECTORY}/testsuite/xbt/graphxml_usage ${PROJECT_DIRECTORY}/testsuite/xbt/graph.xml)
add_test(test-xbt-heap		${PROJECT_DIRECTORY}/testsuite/xbt/heap_bench)

add_test(test-simdag-1 ${PROJECT_DIRECTORY}/testsuite/simdag/sd_test --cfg=path:${PROJECT_DIRECTORY}/testsuite/simdag small_platform_variable.xml)
add_test(test-simdag-2 ${PROJECT_DIRECTORY}/testsuite/simdag/sd_test --cfg=path:${PROJECT_DIRECTORY}/testsuite/simdag ${PROJECT_DIRECTORY}/examples/msg/small_platform.xml)
add_test(test-simdag-3 ${PROJECT_DIRECTORY}/testsuite/simdag/sd_test --cfg=path:${PROJECT_DIRECTORY}/testsuite/simdag ${PROJECT_DIRECTORY}/examples/msg/msg_platform.xml)

add_test(testall		${PROJECT_DIRECTORY}/src/testall)