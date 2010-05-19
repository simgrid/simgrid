cmake_minimum_required(VERSION 2.6)
project(simgrid CXX C)
set(CMAKE_C_FLAGS "" CACHE TYPE INTERNAL FORCE)
set(CMAKE_CXX_FLAGS "" CACHE TYPE INTERNAL FORCE)
set(CMAKE_EXE_LINKER_FLAGS "" CACHE TYPE INTERNAL FORCE)
set(release_version "3.5")
set(libsimgrid_version "${release_version}")
set(libgras_version "${release_version}")
set(libsmpi_version "${release_version}")
exec_program("${CMAKE_COMMAND} -E remove -f ${simgrid_BINARY_DIR}/CTestTestfile.cmake" OUTPUT_VARIABLE rmok)

if(WIN32)
    message("C_COMPILER			$ENV{CC}")
    message("CXX_COMPILER		$ENV{CXX}")
    message("MAKE_PROGRAM		${CMAKE_MAKE_PROGRAM}")
    message("RC_COMPILER		$ENV{RC}")
    message("INCLUDE			$ENV{INCLUDE}")
    message("LIB				$ENV{LIB}")
endif(WIN32)

### Find programs and paths
find_path(GCOV_PATH NAMES gcov PATHS NO_DEFAULT_PATHS)
find_path(VALGRIND_PATH	NAMES valgrind	PATHS NO_DEFAULT_PATHS)

### SET THE PROJECT_DIRECTORY
SET(PROJECT_DIRECTORY "${CMAKE_HOME_DIRECTORY}")

### Set some variables for Cmake
#SET(PROJECT_SOURCE_DIRECTORY "${PROJECT_DIRECTORY}/src") #this one is evil since it speaks to CPack instead
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(INCLUDES "-I${PROJECT_DIRECTORY}/include -I${PROJECT_DIRECTORY}/src/include -I${PROJECT_DIRECTORY}/src -I${CMAKE_CURRENT_BINARY_DIR}/src/")
set(CMAKE_SOURCE_DIR ${PROJECT_SOURCE_DIRECTORY})

if(EXISTS /usr/include/)
	set(INCLUDES "${INCLUDES} -I/usr/include/")
endif(EXISTS /usr/include/)

if(WIN32)
	set(INCLUDES "${INCLUDES} -I$ENV{INCLUDE}")
endif(WIN32)

if(enable_coverage)
	add_definitions (${INCLUDES} -fprofile-arcs -ftest-coverage)
else(enable_coverage)
	add_definitions (${INCLUDES})
endif(enable_coverage)

### Setup Options
include(${PROJECT_DIRECTORY}/buildtools/Cmake/src/CMakeOption.txt)

### Make the *.h files with *.h.in files
include(${PROJECT_DIRECTORY}/buildtools/Cmake/src/CMakeCompleteInFiles.txt)

### Define source packages for Libs 
include(${PROJECT_DIRECTORY}/buildtools/Cmake/src/CMakeDefinePackages.txt)

include(${PROJECT_DIRECTORY}/buildtools/Cmake/src/CMakeMaintainerMode.txt)

### Setup gcc flags
include(${PROJECT_DIRECTORY}/buildtools/Cmake/src/CMakeFlags.txt)

### Make Libs and Exes
include(${PROJECT_DIRECTORY}/buildtools/Cmake/src/CMakeMakeExeLib.txt)

### Make Simgrid.jar and Java examples
if(HAVE_JAVA)
	include(${PROJECT_DIRECTORY}/buildtools/Cmake/src/CMakeJarAndClass.txt)
endif(HAVE_JAVA)

### Make tests
include(${PROJECT_DIRECTORY}/buildtools/Cmake/src/CMakeTest.txt)
include(${PROJECT_DIRECTORY}/buildtools/Cmake/src/CTestConfig.cmake)

### Setup the distrib
include(${PROJECT_DIRECTORY}/buildtools/Cmake/src/CMakeDistrib.txt)

### Print ARGS
include(${PROJECT_DIRECTORY}/buildtools/Cmake/src/CMakePrintArgs.txt)

INCLUDE(Dart)

exec_program("ctest -N" OUTPUT_VARIABLE LIST_OF_TEST)
string(REPLACE "\n" ";" LIST_OF_TEST ${LIST_OF_TEST})
foreach(test ${LIST_OF_TEST})
	string(REGEX REPLACE "^.*Test.*project.*" " " test ${test})
	string(REGEX REPLACE "^.*Total.*Tests:.*" " " test ${test})
	string(REGEX MATCH "^.*Testing.*" test_ok_1 ${test}) #for ctest version 2.6 (xxx/xxx Testing)
	string(REGEX MATCH "^.*Test.*\\:+" test_ok_2 ${test}) #for ctest version 2.8 (Test #xxx)
	if(test_ok_1 OR test_ok_2)
		string(REGEX REPLACE "^.*Testing" " " test ${test}) #for ctest version 2.6
		string(REGEX REPLACE "^.*Test.*\\:+" " " test ${test}) #for ctest version 2.8
		string(STRIP ${test} test)
		set_tests_properties(${test} PROPERTIES LABELS "rev${SVN_VERSION}")
	endif(test_ok_1 OR test_ok_2)
endforeach(test ${LIST_OF_TEST})
