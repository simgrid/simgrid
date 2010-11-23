cmake_minimum_required(VERSION 2.6)

find_path(GCOV_PATH NAMES gcov PATHS NO_DEFAULT_PATHS)
find_path(VALGRIND_PATH	NAMES valgrind	PATHS NO_DEFAULT_PATHS)
find_program(PWD_EXE NAMES pwd)
find_program(SED_EXE NAMES sed)

### AUTO DETECT THE PROJECT_DIRECTORY
exec_program("${PWD_EXE}" ARGS "| ${SED_EXE} 's/\\/Cmake//g'" OUTPUT_VARIABLE PROJECT_DIRECTORY)
### MANUAL PROJECT_DIRECTORY
#set(PROJECT_DIRECTORY "")

SET(CTEST_PROJECT_NAME "Simgrid")

SET(CTEST_BUILD_NAME "CTEST_UCONTEXT")
SET(CTEST_SITE "Pierre_Navarro")

SET(CTEST_SOURCE_DIRECTORY "./")
SET(CTEST_BINARY_DIRECTORY "./")
SET(CTEST_CUSTOM_MAXIMUM_FAILED_TEST_OUTPUT_SIZE "3000000")
SET(CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE "3000000")

SET(CTEST_CMAKE_COMMAND "/home/navarrop/Programmes/cmake-2.8.0/bin/cmake ./")
SET(CTEST_CONFIGURE_COMMAND "/home/navarrop/Programmes/cmake-2.8.0/bin/cmake -Denable_coverage=on -Denable_memcheck=on ./")
SET(CTEST_COMMAND "/home/navarrop/Programmes/cmake-2.8.0/bin/ctest")
SET(CTEST_BUILD_COMMAND "/usr/bin/make -j3")
SET(CTEST_COVERAGE_COMMAND "${GCOV_PATH}/gcov")
SET(CTEST_VALGRIND_COMMAND "${VALGRIND_PATH}/valgrind")
SET(CTEST_MEMORYCHECK_COMMAND "${VALGRIND_PATH}/valgrind")
set(CTEST_MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full --show-reachable=yes --track-origins=yes --read-var-info=no")

if(IS_DIRECTORY ${PROJECT_DIRECTORY}/.svn)
	SET(CTEST_UPDATE_COMMAND "/usr/bin/svn")
endif(IS_DIRECTORY ${PROJECT_DIRECTORY}/.svn)

if(IS_DIRECTORY ${PROJECT_DIRECTORY}/.git)
	SET(CTEST_UPDATE_COMMAND "/usr/bin/git")
endif(IS_DIRECTORY ${PROJECT_DIRECTORY}/.git)

SET(CTEST_DROP_METHOD "http")
SET(CTEST_DROP_SITE "cdash.inria.fr/CDash")
SET(CTEST_DROP_LOCATION "/submit.php?project=${CTEST_PROJECT_NAME}")
SET(CTEST_DROP_SITE_CDASH TRUE)
SET(CTEST_TRIGGER_SITE "http://cdash.inria.fr/CDash/cgi-bin/Submit-Random-TestingResults.cgi")

###Custom ctest

#CTEST_CUSTOM_ERROR_MATCH 	 		Regular expression for errors during build process
#CTEST_CUSTOM_ERROR_EXCEPTION 			Regular expression for error exceptions during build process
#CTEST_CUSTOM_WARNING_MATCH 			Regular expression for warnings during build process
#CTEST_CUSTOM_WARNING_EXCEPTION 		Regular expression for warning exception during build process
#CTEST_CUSTOM_MAXIMUM_NUMBER_OF_ERRORS 		Maximum number of errors to display
#CTEST_CUSTOM_MAXIMUM_NUMBER_OF_WARNINGS 	Maximum number of warnings to display
#CTEST_CUSTOM_TESTS_IGNORE 			List of tests to ignore during the Test stage
#CTEST_CUSTOM_MEMCHECK_IGNORE 			List of tests to ignore during the MemCheck stage
#CTEST_CUSTOM_PRE_TEST 				Command to execute before any tests are run during Test stage
#CTEST_CUSTOM_POST_TEST 			Command to execute after any tests are run during Test stage
#CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE 	Maximum size of passed test output
#CTEST_CUSTOM_MAXIMUM_FAILED_TEST_OUTPUT_SIZE 	Maximum size of failed test output
#CTEST_CUSTOM_PRE_MEMCHECK 			Command to execute before any tests are run during MemCheck stage
#CTEST_CUSTOM_POST_MEMCHECK 			Command to execute after any tests are run during MemCheck stage
#CTEST_CUSTOM_COVERAGE_EXCLUDE 	 		Regular expression for excluding files from coverage testing 

SET(CTEST_CUSTOM_COVERAGE_EXCLUDE
"${PROJECT_DIRECTORY}/tools/*"
"${PROJECT_DIRECTORY}/buildtools/*"
"${PROJECT_DIRECTORY}/include/*"
"${PROJECT_DIRECTORY}/examples/*"
"${PROJECT_DIRECTORY}/testsuite/*"
"${PROJECT_DIRECTORY}/teshsuite/*"
"${PROJECT_DIRECTORY}/src/bindings/*"
)

#ignore some memcheck tests
set(CTEST_CUSTOM_MEMCHECK_IGNORE
	tesh-self-basic
	tesh-self-cd
	tesh-self-IO-broken-pipe
	tesh-self-IO-orders
	tesh-self-IO-bigsize
	tesh-self-set-return
	tesh-self-set-signal
	tesh-self-set-timeout
	tesh-self-set-ignore-output
	tesh-self-catch-return
	tesh-self-catch-signal
	tesh-self-catch-timeout
	tesh-self-catch-wrong-output
	tesh-self-bg-basic
	tesh-self-bg-set-signal
	tesh-self-background
	tesh-log-large
	tesh-log-parallel
	tesh-gras-dd-mem
	tesh-gras-dd-rw
	tesh-gras-dd-r_little32_4
	tesh-gras-dd-r_little64
	tesh-gras-dd-r_big32_8_4
	tesh-gras-dd-r_big32_8
	tesh-gras-dd-r_big32_2
	tesh-gras-empty_main-rl
	tesh-gras-empty_main-sg
	tesh-simdag-reinit_costs
	tesh-simdag-parser
	tesh-simdag-flatifier
	tesh-simdag-basic0
	tesh-simdag-basic1
	tesh-simdag-basic2
	tesh-simdag-basic3
	tesh-simdag-basic4
	tesh-simdag-basic5
	tesh-simdag-basic6
	tesh-simdag-p2p-1
	tesh-simdag-p2p-2
	tesh-simdag-p2p-3
	tesh-simdag-p2p-3
	tesh-simdag-mxn-1
	tesh-simdag-mxn-2
	tesh-simdag-mxn-3
	tesh-simdag-par-1
	tesh-simdag-par-2
	tesh-msg-get_sender
	gras-ping-rl
	gras-rpc-rl
	gras-spawn-rl
	gras-timer-rl
	gras-chrono-rl
	gras-simple_token-rl
	gras-mmrpc-rl
	gras-all2all-rl
	gras-pmm-rl
	gras-synchro-rl
	gras-properties-rl
	msg-sendrecv_CLM03
	msg-sendrecv_Vegas
	msg-sendrecv_Reno
	msg-suspend
	msg-masterslave
	msg-masterslave-forwarder
	msg-masterslave-failure
	msg-masterslave-bypass
	msg-migration
	msg-ptask
	msg-priority
	msg-properties
	msg-trace
	msg-masterslave_cpu_ti
	gras-properties-sg
	amok-bandwidth-rl
	amok-saturate-rl
	simdag-test_simdag
	simdag-test_simdag2
	simdag-test_prop
	simdag-metaxml_test
	smpi-bcast
	smpi-reduce
)

IF(${ARCH_32_BITS})
	SET(CTEST_CUSTOM_MEMCHECK_IGNORE
	${CTEST_CUSTOM_MEMCHECK_IGNORE}
	tesh-gras-msg_handle-sg-32
	tesh-gras-small_sleep-sg-32
	gras-ping-sg-32
	gras-rpc-sg-32
	gras-spawn-sg-32
	gras-timer-sg-32
	gras-chrono-sg-32
	gras-simple_token-sg-32
	gras-mmrpc-sg-32
	gras-all2all-sg-32
	gras-pmm-sg-32
	gras-synchro-sg-32
	amok-bandwidth-sg-32
	amok-saturate-sg-32
	)
ELSE(${ARCH_32_BITS})
	SET(CTEST_CUSTOM_MEMCHECK_IGNORE
	${CTEST_CUSTOM_MEMCHECK_IGNORE}
	tesh-gras-msg_handle-sg-64
	tesh-gras-small_sleep-sg-64
	gras-ping-sg-64
	gras-rpc-sg-64
	gras-spawn-sg-64
	gras-timer-sg-64
	gras-chrono-sg-64
	gras-simple_token-sg-64
	gras-mmrpc-sg-64
	gras-all2all-sg-64
	gras-pmm-sg-64
	gras-synchro-sg-64
	amok-bandwidth-sg-64
	amok-saturate-sg-64
	)
ENDIF(${ARCH_32_BITS})

if(HAVE_GTNETS)
	SET(CTEST_CUSTOM_MEMCHECK_IGNORE
	${CTEST_CUSTOM_MEMCHECK_IGNORE}
	msg-gtnets1
	msg-gtnets2
	msg-gtnets3
	msg-gtnets4
	msg-gtnets5
	)
endif(HAVE_GTNETS)

if(HAVE_JAVA)
	SET(CTEST_CUSTOM_MEMCHECK_IGNORE
	${CTEST_CUSTOM_MEMCHECK_IGNORE}
	java-basic
	java-pingpong
	java-comm_time
	java-suspend
	)
endif(HAVE_JAVA)

ctest_start(Experimental)
ctest_update(SOURCE "${CTEST_SOURCE_DIRECTORY}")
ctest_configure(BUILD "${CTEST_SOURCE_DIRECTORY}") 
ctest_build(BUILD "${CTEST_SOURCE_DIRECTORY}") 
ctest_test(BUILD "${CTEST_SOURCE_DIRECTORY}")
ctest_coverage(BUILD "${CTEST_SOURCE_DIRECTORY}")
ctest_memcheck(BUILD "${CTEST_SOURCE_DIRECTORY}")
ctest_submit()
