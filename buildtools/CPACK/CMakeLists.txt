cmake_minimum_required(VERSION 2.6)

find_program(SH_EXE NAMES sh)
find_program(SED_EXE NAMES sed)
find_program(PWD_EXE NAMES pwd)
find_program(UNAME NAMES uname)
find_program(HOSTNAME_CMD NAMES hostname)
find_path(GCC_PATH NAMES gcc PATHS NO_DEFAULT_PATHS	)
find_path(GCOV_PATH NAMES gcov PATHS NO_DEFAULT_PATHS	)
find_path(VALGRIND_PATH	NAMES valgrind	PATHS NO_DEFAULT_PATHS	)

### DETECT AUTO. THE PROJECT_DIRECTORY
exec_program("${PWD_EXE}" ARGS "| ${SED_EXE} 's/\\/CPACK//g'" OUTPUT_VARIABLE PROJECT_DIRECTORY)

### DETECT MANUAL. THE PROJECT_DIRECTORY
#SET(PROJECT_DIRECTORY "/home/navarrop/Bureau/simgrid-trunk")

### ARGs use -D[var]=[ON/OFF] or [1/0] or [true/false](see after)
### ex: cmake -Ddisable_java=ON -Ddisable_gtnets=ON ./

SET(disable_java OFF) #default value
SET(disable_threads ON)
SET(disable_gtnets ON)
SET(disable_context ON)
SET(disable_lua ON)

option(disable_java "This variable set the java context." OFF)
option(disable_gtnets "This variable set the gtnets context." ON)
option(disable_threads "This variable set the thread context." ON)
option(disable_context "This variable set the context." ON)
option(disable_lua "This variable set lua use." ON)

message("\n________ARGS_________")

message("disable_java ${disable_java}")

message("disable_gtnets ${disable_gtnets}")

message("disable_threads ${disable_threads}")
if(disable_gtnets)
	set(gtnets_path "none" CACHE TYPE INTERNAL FORCE)	
endif(disable_gtnets)
message("  -> gtnets_path \"${gtnets_path}\"")

message("disable_context ${disable_context}")
if(disable_context)
	set(with_context "none" CACHE TYPE INTERNAL FORCE)	
endif(disable_context)
message("  -> with_context \"${with_context}\"")

message("disable_lua ${disable_lua}")

message("_______END_ARGS_______")
#--------------------------------------------------------------------------------------------------

### Set some variables for Cmake
SET(PROJECT_NAME "Simgrid")
SET(LANGUAGE "C")
SET(PROJECT_SOURCE_DIRECTORY "${PROJECT_DIRECTORY}/src")
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_DIRECTORY}/bin")
set(LIBRARY_OUTPUT_PATH "${PROJECT_DIRECTORY}/lib")
set(INCLUDES "-I${PROJECT_DIRECTORY}/include -I${PROJECT_DIRECTORY}/src/include -I${PROJECT_DIRECTORY}/src")
set(CMAKE_SOURCE_FILES ${PROJECT_SOURCE_DIRECTORY})
set(CMAKE_CXX_FLAGS "-fprofile-arcs -ftest-coverage")
set(CMAKE_C_FLAGS "-fprofile-arcs -ftest-coverage")
add_definitions (-fprofile-arcs -ftest-coverage)
add_definitions("${INCLUDES}")

### Make the *.h files with *.h.in files
include(${PROJECT_DIRECTORY}/CPACK/CMakeCompleteInFiles.txt OPTIONAL NO_POLICY_SCOPE)

### Define source packages for Libs 
include(${PROJECT_DIRECTORY}/CPACK/CMakeDefinePackages.txt OPTIONAL NO_POLICY_SCOPE)

### Make tests
include(${PROJECT_DIRECTORY}/CPACK/CMakeTest.txt OPTIONAL NO_POLICY_SCOPE)

### Make Libs
add_library(simgrid STATIC ${simgrid_sources})
add_library(gras STATIC ${gras_sources})
set(GRAS_DEP "")
set(SIMGRID_DEP "")

if(pthread)
	if(with_context MATCHES pthread)
		SET(SIMGRID_DEP pthread)
		SET(ESPACE " ")
	endif(with_context MATCHES pthread)

	if(with_context MATCHES windows)
	elseif(with_context MATCHES windows)
		SET(GRAS_DEP pthread)	
	endif(with_context MATCHES windows)
endif(pthread)

if(HAVE_LUA)
	SET(SIMGRID_DEP "${SIMGRID_DEP}${ESPACE}dl lua5.1")
endif(HAVE_LUA)

message("SIMGRID_DEP \"${SIMGRID_DEP}\"")
message("GRAS_DEP \"${GRAS_DEP}\"")
target_link_libraries(simgrid ${SIMGRID_DEP} -fprofile-arcs)
target_link_libraries(gras ${GRAS_DEP} -fprofile-arcs)

### Make EXEs
add_executable(masterslave_failure "${PROJECT_DIRECTORY}/examples/msg/masterslave/masterslave_failure.c")
add_executable(masterslave_forwarder "${PROJECT_DIRECTORY}/examples/msg/masterslave/masterslave_forwarder.c")
add_executable(Test "${PROJECT_DIRECTORY}/src/test.c")
target_link_libraries(masterslave_forwarder simgrid m -fprofile-arcs)
target_link_libraries(masterslave_failure simgrid m -fprofile-arcs)
target_link_libraries(Test -fprofile-arcs)

### Make the distrib
include(${PROJECT_DIRECTORY}/CPACK/CMakeDistrib.txt OPTIONAL NO_POLICY_SCOPE)

INCLUDE(Dart)
