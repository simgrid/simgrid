cmake_minimum_required(VERSION 2.6)
SET(CMAKE_LANGUAGE "Cxx")
SET(PROJECT_NAME "Simgrid")

### Find programs and paths
find_program(SH_EXE NAMES sh)
find_program(SED_EXE NAMES sed)
find_program(PWD_EXE NAMES pwd)
find_program(UNAME NAMES uname)
find_program(HOSTNAME_CMD NAMES hostname)
find_program(CAT NAMES cat)
find_path(GCC_PATH NAMES gcc PATHS NO_DEFAULT_PATHS)
find_path(GCOV_PATH NAMES gcov PATHS NO_DEFAULT_PATHS)
find_path(VALGRIND_PATH	NAMES valgrind	PATHS NO_DEFAULT_PATHS)

### AUTO DETECT THE PROJECT_DIRECTORY
exec_program("${PWD_EXE}" ARGS "| ${SED_EXE} 's/\\/CPACK//g'" OUTPUT_VARIABLE PROJECT_DIRECTORY)

### SET THE PROJECT_DIRECTORY
#SET(PROJECT_DIRECTORY "/home/navarrop/Bureau/simgrid-trunk")

### Set some variables for Cmake
SET(PROJECT_SOURCE_DIRECTORY "${PROJECT_DIRECTORY}/src")
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/bin")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/src/.libs")
set(INCLUDES "-I${PROJECT_DIRECTORY}/include -I${PROJECT_DIRECTORY}/src/include -I${PROJECT_DIRECTORY}/src -I/usr/include -L${gtnets_path}/lib")
set(CMAKE_SOURCE_DIR ${PROJECT_SOURCE_DIRECTORY})

add_definitions (${INCLUDES} -fprofile-arcs -ftest-coverage)

### Setup Options
include(${PROJECT_DIRECTORY}/CPACK/src/CMakeOption.txt)

### Make the *.h files with *.h.in files
include(${PROJECT_DIRECTORY}/CPACK/src/CMakeCompleteInFiles.txt)

### Define source packages for Libs 
include(${PROJECT_DIRECTORY}/CPACK/src/CMakeDefinePackages.txt)

### Setup gcc flags
include(${PROJECT_DIRECTORY}/CPACK/src/CMakeFlags.txt)

### Make Libs and Exes
include(${PROJECT_DIRECTORY}/CPACK/src/CMakeMakeExeLib.txt)

### Make tests
include(${PROJECT_DIRECTORY}/CPACK/src/CMakeTest.txt)
include(${PROJECT_DIRECTORY}/CPACK/src/CTestConfig.cmake)

### Setup the distrib
include(${PROJECT_DIRECTORY}/CPACK/src/CMakeDistrib.txt)

### Print ARGS
include(${PROJECT_DIRECTORY}/CPACK/src/CMakePrintArgs.txt)

INCLUDE(Dart)
