cmake_minimum_required(VERSION 2.6)
find_program(SH_EXE NAMES sh)
find_program(SED_EXE NAMES sed)
find_program(PWD_EXE NAMES pwd)
find_program(UNAME NAMES uname)
find_program(HOSTNAME_CMD NAMES hostname)
find_path(GCC_PATH	NAMES gcc	PATHS NO_DEFAULT_PATHS	)
find_path(GCOV_PATH	NAMES gcov	PATHS NO_DEFAULT_PATHS	)
find_path(VALGRIND_PATH	NAMES valgrind	PATHS NO_DEFAULT_PATHS	)

### DETECT AUTO. THE PROJECT_DIRECTORY
exec_program("${PWD_EXE}" ARGS "| ${SED_EXE} 's/\\/CPACK//g'" OUTPUT_VARIABLE "PROJECT_DIRECTORY")

### DETECT MANUAL. THE PROJECT_DIRECTORY
#SET(PROJECT_DIRECTORY "/home/navarrop/Bureau/simgrid-trunk")

### Get the hostname of current machine :
exec_program(${HOSTNAME_CMD} OUTPUT_VARIABLE SITE)

### Get the system information of current machine
exec_program("${UNAME}" OUTPUT_VARIABLE "SYSTEM")

### ARGs use -D[var]=[ON/OFF] or [1/0] or [true/false](see after)
### ex: cmake -Ddisable_java=ON -Ddisable_gtnets=ON ./

SET(disable_java OFF) #default value
SET(disable_threads ON)
SET(disable_gtnets ON)

option(disable_java "This variable set the java context.")
option(disable_gtnets "This variable set the gtnets context.")
option(disable_threads "This variable set the thread context.")

message("\n________ARGS_________")
message("disable_java ${disable_java}")
message("disable_gtnets ${disable_gtnets}")
if(NOT disable_gtnets)
message("gtnets_path \"${gtnets_path}\"")
endif(NOT disable_gtnets)
message("disable_threads ${disable_threads}")
message("_______END_ARGS_______")
#--------------------------------------------------------------------------------------------------

### Set some variables for Cmake
SET(PROJECT_NAME "Simgrid")
SET(PROJECT_SOURCE_DIRECTORY "${PROJECT_DIRECTORY}/src")
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/bin")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")
set(INCLUDES "-I${PROJECT_DIRECTORY}/include -I${PROJECT_DIRECTORY}/src/include -I${PROJECT_DIRECTORY}/src")

add_definitions("${INCLUDES}")
add_definitions (-fprofile-arcs)
add_definitions (-ftest-coverage)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")

### Make the *.h files with *.h.in files
include(${PROJECT_DIRECTORY}/CPACK/CMakeCompleteInFiles.txt OPTIONAL NO_POLICY_SCOPE)

### Define source packages for Libs 
include(${PROJECT_DIRECTORY}/CPACK/CMakeDefinePackages.txt OPTIONAL NO_POLICY_SCOPE)

### Make Libs
add_library(simgrid SHARED ${simgrid_sources})
add_library(gras SHARED ${gras_sources})
### Make EXEs
add_executable(masterslave_failure "${PROJECT_DIRECTORY}/examples/msg/masterslave/masterslave_failure.c")
add_executable(masterslave_forwarder "${PROJECT_DIRECTORY}/examples/msg/masterslave/masterslave_forwarder.c")
target_link_libraries(masterslave_forwarder simgrid m gcov)
target_link_libraries(masterslave_failure simgrid m gcov)


### Make tests
include(${PROJECT_DIRECTORY}/CPACK/CMakeTest.txt OPTIONAL NO_POLICY_SCOPE)

### Make the distrib
include(${PROJECT_DIRECTORY}/CPACK/CMakeDistrib.txt OPTIONAL NO_POLICY_SCOPE)


