cmake_minimum_required(VERSION 2.6)

find_path(GCOV_PATH NAMES gcov PATHS NO_DEFAULT_PATHS)
find_path(VALGRIND_PATH	NAMES valgrind	PATHS NO_DEFAULT_PATHS)
find_program(PWD_EXE NAMES pwd)
find_program(SED_EXE NAMES sed)

### AUTO DETECT THE CMAKE_HOME_DIRECTORY
exec_program("${PWD_EXE}" ARGS "| ${SED_EXE} 's/\\/Cmake//g'" OUTPUT_VARIABLE CMAKE_HOME_DIRECTORY)
### MANUAL CMAKE_HOME_DIRECTORY
#set(CMAKE_HOME_DIRECTORY "")

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

if(IS_DIRECTORY ${CMAKE_HOME_DIRECTORY}/.svn)
  SET(CTEST_UPDATE_COMMAND "/usr/bin/svn")
endif()

if(IS_DIRECTORY ${CMAKE_HOME_DIRECTORY}/.git)
  SET(CTEST_UPDATE_COMMAND "/usr/bin/git")
endif()

SET(CTEST_DROP_METHOD "http")
SET(CTEST_DROP_SITE "cdash.inria.fr/CDash")
SET(CTEST_DROP_LOCATION "/submit.php?project=${CTEST_PROJECT_NAME}")
SET(CTEST_DROP_SITE_CDASH TRUE)
SET(CTEST_TRIGGER_SITE "http://cdash.inria.fr/CDash/cgi-bin/Submit-Random-TestingResults.cgi")

###Custom ctest

#CTEST_CUSTOM_ERROR_MATCH                       Regular expression for errors during build process
#CTEST_CUSTOM_ERROR_EXCEPTION                   Regular expression for error exceptions during build process
#CTEST_CUSTOM_WARNING_MATCH                     Regular expression for warnings during build process
#CTEST_CUSTOM_WARNING_EXCEPTION                 Regular expression for warning exception during build process
#CTEST_CUSTOM_MAXIMUM_NUMBER_OF_ERRORS          Maximum number of errors to display
#CTEST_CUSTOM_MAXIMUM_NUMBER_OF_WARNINGS        Maximum number of warnings to display
#CTEST_CUSTOM_TESTS_IGNORE                      List of tests to ignore during the Test stage
#CTEST_CUSTOM_MEMCHECK_IGNORE                   List of tests to ignore during the MemCheck stage
#CTEST_CUSTOM_PRE_TEST                          Command to execute before any tests are run during Test stage
#CTEST_CUSTOM_POST_TEST                         Command to execute after any tests are run during Test stage
#CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE   Maximum size of passed test output
#CTEST_CUSTOM_MAXIMUM_FAILED_TEST_OUTPUT_SIZE   Maximum size of failed test output
#CTEST_CUSTOM_PRE_MEMCHECK                      Command to execute before any tests are run during MemCheck stage
#CTEST_CUSTOM_POST_MEMCHECK                     Command to execute after any tests are run during MemCheck stage
#CTEST_CUSTOM_COVERAGE_EXCLUDE                  Regular expression for excluding files from coverage testing

SET(CTEST_CUSTOM_COVERAGE_EXCLUDE
  "${CMAKE_HOME_DIRECTORY}/tools/"
  "${CMAKE_HOME_DIRECTORY}/buildtools/"
  "${CMAKE_HOME_DIRECTORY}/include/"
  "${CMAKE_HOME_DIRECTORY}/examples/"
  "${CMAKE_HOME_DIRECTORY}/testsuite/"
  "${CMAKE_HOME_DIRECTORY}/teshsuite/"
  "${CMAKE_HOME_DIRECTORY}/src/bindings/"
  )

ctest_start(Experimental)
ctest_update(SOURCE "${CTEST_SOURCE_DIRECTORY}")
ctest_configure(BUILD "${CTEST_SOURCE_DIRECTORY}")
ctest_build(BUILD "${CTEST_SOURCE_DIRECTORY}")
ctest_test(BUILD "${CTEST_SOURCE_DIRECTORY}")
ctest_coverage(BUILD "${CTEST_SOURCE_DIRECTORY}")
ctest_memcheck(BUILD "${CTEST_SOURCE_DIRECTORY}")
ctest_submit()
