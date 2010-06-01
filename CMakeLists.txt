cmake_minimum_required(VERSION 2.6)
project(simgrid CXX C)
set(CMAKE_C_FLAGS "" CACHE TYPE INTERNAL FORCE)
set(CMAKE_CXX_FLAGS "" CACHE TYPE INTERNAL FORCE)
set(CMAKE_EXE_LINKER_FLAGS "" CACHE TYPE INTERNAL FORCE)
set(release_version "3.5")
set(libsimgrid_version "${release_version}")
set(libgras_version "${release_version}")
set(libsmpi_version "${release_version}")

### Find programs and paths
find_path(GCOV_PATH NAMES gcov PATHS NO_DEFAULT_PATHS)
find_path(VALGRIND_PATH	NAMES valgrind	PATHS NO_DEFAULT_PATHS)

### SET THE PROJECT_DIRECTORY
SET(PROJECT_DIRECTORY "${CMAKE_HOME_DIRECTORY}")

### Set some variables for Cmake
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(INCLUDES "-I${PROJECT_DIRECTORY}/include -I${PROJECT_DIRECTORY}/src/include -I${PROJECT_DIRECTORY}/src -I${CMAKE_CURRENT_BINARY_DIR}/src/")
set(CMAKE_SOURCE_DIR ${PROJECT_SOURCE_DIRECTORY})

if(EXISTS /usr/include/)
	set(INCLUDES "${INCLUDES} -I/usr/include/")
endif(EXISTS /usr/include/)

if(WIN32)
	set(INCLUDES_MORE $ENV{INCLUDE})
	string(REPLACE "\\" "/" INCLUDES_MORE "${INCLUDES_MORE}")
	foreach(include_dir ${INCLUDES_MORE})
		set(INCLUDES "${INCLUDES} -I\"${include_dir}\"")
	endforeach(include_dir ${INCLUDES_MORE})
endif(WIN32)

if(WIN32)
	message("	Be sure all those variables are correctely set.")
    message("		-- C_COMPILER			$ENV{CC}")
    message("		-- CXX_COMPILER			$ENV{CXX}")    
    message("		-- RC_COMPILER			$ENV{RC}")
    message("		-- INCLUDE			$ENV{INCLUDE}")
    message("		-- LIB				$ENV{LIB}")
    message("		-- MAKE_PROGRAM			${CMAKE_MAKE_PROGRAM}")
    message("		-- CMAKE_BUILD_TOOL		${CMAKE_BUILD_TOOL}")
    message("		-- LINKER			${CMAKE_LINKER}")
    message("		-- CMAKE_GENERATOR		${CMAKE_GENERATOR}\n\n")
    message("		-- INCLUDES			${INCLUDES}")
    set(__STRICT_ANSI__ 1)
    set(_MSC_VER 1)
    set(_WIN32 1)
endif(WIN32)

if(enable_coverage)
	add_definitions (${INCLUDES} -fprofile-arcs -ftest-coverage)
else(enable_coverage)
	add_definitions (${INCLUDES})
endif(enable_coverage)

### Setup Options
include(${PROJECT_DIRECTORY}/buildtools/Cmake/Option.cmake)

### Make the *.h files with *.h.in files
include(${PROJECT_DIRECTORY}/buildtools/Cmake/CompleteInFiles.cmake)

### Define source packages for Libs 
include(${PROJECT_DIRECTORY}/buildtools/Cmake/DefinePackages.cmake)

include(${PROJECT_DIRECTORY}/buildtools/Cmake/MaintainerMode.cmake)

### Setup gcc flags
include(${PROJECT_DIRECTORY}/buildtools/Cmake/Flags.cmake)

### Make Libs and Exes
include(${PROJECT_DIRECTORY}/buildtools/Cmake/MakeExeLib.cmake)

### Make Simgrid.jar and Java examples
if(HAVE_JAVA)
	include(${PROJECT_DIRECTORY}/buildtools/Cmake/MakeJava.cmake)
endif(HAVE_JAVA)

### Make tests
include(${PROJECT_DIRECTORY}/buildtools/Cmake/AddTests.cmake)
include(${PROJECT_DIRECTORY}/buildtools/Cmake/CTestConfig.cmake)

### Setup the distrib
include(${PROJECT_DIRECTORY}/buildtools/Cmake/Distrib.cmake)

### Print ARGS
include(${PROJECT_DIRECTORY}/buildtools/Cmake/PrintArgs.cmake)

INCLUDE(Dart)