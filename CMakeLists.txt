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
FIND_PROGRAM(GCOV_PATH gcov)
FIND_PROGRAM(VALGRIND_PATH valgrind)
include(FindPerl)
if(NOT PERL_EXECUTABLE)
	message(FATAL_ERROR "-- SimGrid cannot be compiled without Perl installed -- sorry. Bailling out.")
endif(NOT PERL_EXECUTABLE)

if(NOT WIN32)
	exec_program("${CMAKE_COMMAND} -E chdir \"${CMAKE_HOME_DIRECTORY}\" pwd -P" OUTPUT_VARIABLE TRUE_CMAKE_HOME_DIRECTORY)
	if(${TRUE_CMAKE_HOME_DIRECTORY} STREQUAL ${CMAKE_HOME_DIRECTORY})
	else(${TRUE_CMAKE_HOME_DIRECTORY} STREQUAL ${CMAKE_HOME_DIRECTORY})
		message(STATUS "Change link path '${CMAKE_HOME_DIRECTORY}' to true path '${TRUE_CMAKE_HOME_DIRECTORY}'")
		set(CMAKE_HOME_DIRECTORY "${TRUE_CMAKE_HOME_DIRECTORY}" CACHE TYPE INTERNAL FORCE)
		set(CMAKE_SOURCE_DIR "${TRUE_CMAKE_HOME_DIRECTORY}" CACHE TYPE INTERNAL FORCE)
	endif(${TRUE_CMAKE_HOME_DIRECTORY} STREQUAL ${CMAKE_HOME_DIRECTORY})
	
	exec_program("${CMAKE_COMMAND} -E chdir \"${CMAKE_BINARY_DIR}\" pwd -P" OUTPUT_VARIABLE TRUE_CMAKE_BINARY_DIR)
	if(${TRUE_CMAKE_BINARY_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	else(${TRUE_CMAKE_BINARY_DIR} STREQUAL ${CMAKE_BINARY_DIR})
		message(STATUS "Change link path '${CMAKE_BINARY_DIR}' to true path '${TRUE_CMAKE_BINARY_DIR}'")
		set(CMAKE_BINARY_DIR "${TRUE_CMAKE_BINARY_DIR}" CACHE TYPE INTERNAL FORCE)
	endif(${TRUE_CMAKE_BINARY_DIR} STREQUAL ${CMAKE_BINARY_DIR})	
endif(NOT WIN32)

### SET THE LIBRARY EXTENSION
if(APPLE)
	set(LIB_EXE "dylib")
else(APPLE)
    if(WIN32)
        set(LIB_EXE "a")
    else(WIN32)
	    set(LIB_EXE "so")
    endif(WIN32)
endif(APPLE)

### Set some variables for Cmake
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_HOME_DIRECTORY}")
	set(INCLUDES
	${CMAKE_HOME_DIRECTORY}
	${CMAKE_HOME_DIRECTORY}/include
	${CMAKE_HOME_DIRECTORY}/src
	${CMAKE_HOME_DIRECTORY}/src/include
	)
else("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_HOME_DIRECTORY}")
	set(INCLUDES
	${CMAKE_HOME_DIRECTORY}
	${CMAKE_HOME_DIRECTORY}/include
	${CMAKE_HOME_DIRECTORY}/src
	${CMAKE_HOME_DIRECTORY}/src/include
	${CMAKE_BINARY_DIR}
	${CMAKE_BINARY_DIR}/include
	${CMAKE_BINARY_DIR}/src
	)
endif("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_HOME_DIRECTORY}")

set(INCLUDES ${INCLUDES} ${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/include ${CMAKE_HOME_DIRECTORY}/src/include ${CMAKE_HOME_DIRECTORY}/src ${CMAKE_BINARY_DIR}/src)
if(WIN32)
    set(INCLUDES ${INCLUDES} ${CMAKE_HOME_DIRECTORY}/include/xbt ${CMAKE_HOME_DIRECTORY}/src/xbt) #for win32_ucontext.[ch]
endif(WIN32)
set(CMAKE_SOURCE_DIR ${PROJECT_SOURCE_DIRECTORY})

if(NOT CMAKE_CROSSCOMPILING AND EXISTS /usr/include/)
	set(INCLUDES ${INCLUDES} /usr/include/)
endif(NOT CMAKE_CROSSCOMPILING AND EXISTS /usr/include/)

### Check 32bits or 64bits
INCLUDE (CheckTypeSize)
CHECK_TYPE_SIZE("long" SIZEOF_LONG)
IF(SIZEOF_LONG EQUAL 4)
  SET(ARCH_32_BITS 1)
ELSE(SIZEOF_LONG EQUAL 4)
  SET(ARCH_32_BITS 0)
ENDIF(SIZEOF_LONG EQUAL 4)

if(WIN32)

	set(INCLUDES_MORE $ENV{INCLUDE})
	string(REPLACE "\\" "/" INCLUDES_MORE "${INCLUDES_MORE}")
	foreach(include_dir ${INCLUDES_MORE})
	    if(IS_DIRECTORY ${include_dir})
	        string(REGEX MATCH ".*[ ].*" testoki "${include_dir}")
	        if(testoki)
	        	string(REPLACE " " "\\ " include_dir "${include_dir}")
	            set(INCLUDES ${INCLUDES} \"${include_dir}\")
	        else(testoki)
	            set(INCLUDES ${INCLUDES} ${include_dir})
	        endif(testoki)
	    else(IS_DIRECTORY ${include_dir})
	        message(FATAL_ERROR "${include_dir} not find")
	    endif(IS_DIRECTORY ${include_dir})
	endforeach(include_dir ${INCLUDES_MORE})

    if(BORLAND)			### BORLAND COMPILER
	    set(__BORLANDC__ 1)
	    set(CMAKE_COMPILER_IS_GNUCC 0)
	    set(MSVC 0)
	    set(_XBT_BORLANDC_COMPILER 1)
    endif(BORLAND)
    
    if(MSVC)	### MicroSoftVisualC COMPILER
	    set(__VISUALC__ 1)
	    set(BORLAND 0)
	    set(CMAKE_COMPILER_IS_GNUCC 0)
	    set(_XBT_VISUALC_COMPILER 1)
	    set(_MSC_VER 1)
        set(__STRICT_ANSI__ 1)
        set(_M_IX86 1)
    endif(MSVC)
    
    if(CMAKE_COMPILER_IS_GNUCC)
        set(__GNUC__ 1)
        exec_program("${CMAKE_C_COMPILER} --version" OUTPUT_VARIABLE "COMPILER_C_VERSION")
        string(REGEX MATCH "[0-9].[0-9].[0-9]" COMPILER_C_VERSION "${COMPILER_C_VERSION}")
        string(REGEX MATCH "^[0-9]" COMPILER_C_MAJOR_VERSION "${COMPILER_C_VERSION}")
        string(REGEX MATCH "^[0-9].[0-9]" COMPILER_C_MINOR_VERSION "${COMPILER_C_VERSION}")
        string(REGEX REPLACE "^${COMPILER_C_MAJOR_VERSION}." "" COMPILER_C_MINOR_VERSION "${COMPILER_C_MINOR_VERSION}")
        if(COMPILER_C_MAJOR_VERSION)
           # set(__GNUC__ ${COMPILER_C_MAJOR_VERSION})
        endif(COMPILER_C_MAJOR_VERSION)
        if(COMPILER_C_MINOR_VERSION)
           # set(__GNUC_MINOR__ ${COMPILER_C_MINOR_VERSION})
        endif(COMPILER_C_MINOR_VERSION)
        set(MSVC 0)
	    set(BORLAND 0)
    endif(CMAKE_COMPILER_IS_GNUCC)
    
    if(ARCH_32_BITS)	### Arch 32bits
        set(_WIN32 1)
    else(ARCH_32_BITS)	### Arch 64bits
        set(_WIN64 1)
    endif(ARCH_32_BITS)
    
    set(_XBT_WIN32 1)
    
    message(STATUS "C_COMPILER			$ENV{CC}")
    message(STATUS "CXX_COMPILER			$ENV{CXX}")    
    message(STATUS "INCLUDE			$ENV{INCLUDE}")
    message(STATUS "LIB				$ENV{LIB}")
    message(STATUS "MAKE_PROGRAM			${CMAKE_MAKE_PROGRAM}")
    message(STATUS "CMAKE_BUILD_TOOL		${CMAKE_BUILD_TOOL}")
    message(STATUS "LINKER			${CMAKE_LINKER}")
    message(STATUS "CMAKE_GENERATOR		${CMAKE_GENERATOR}")
    message(STATUS "BORLAND			${BORLAND}")
    message(STATUS "VISUALC			${MSVC}")
    message(STATUS "GNUC			        ${CMAKE_COMPILER_IS_GNUCC}")
   
endif(WIN32)

include_directories(${INCLUDES})


### Determine the assembly flavor that we need today
include(CMakeDetermineSystem)
set(PROCESSOR_${CMAKE_SYSTEM_PROCESSOR} 1)

### Setup Options
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/Option.cmake)

### Make the *.h files with *.h.in files
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/CompleteInFiles.cmake)

### Define source packages for Libs 
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/DefinePackages.cmake)

### Build some Maintainer files
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/MaintainerMode.cmake)
if(NOT WIN32)
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/UnitTesting.cmake)
endif(NOT WIN32)

### Setup gcc flags
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/Flags.cmake)

### Make Libs
if(NOT WIN32)
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/MakeLib.cmake)
else(NOT WIN32)
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/MakeLibWin.cmake)
endif(NOT WIN32)

### Make Exes
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/MakeExe.cmake)

### Make Simgrid.jar and Java examples
if(HAVE_JAVA)
	include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/MakeJava.cmake)
endif(HAVE_JAVA)

### Make tests
if(NOT WIN32)
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/AddTests.cmake)
else(NOT WIN32)

endif(NOT WIN32)

include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/CTestConfig.cmake)

### Setup the distrib
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/Distrib.cmake)

### Pipol compilation
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/Pipol.cmake)

if(NOT WIN32)
### Build the doc
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/GenerateDoc.cmake)
endif(NOT WIN32)
### Print ARGS
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/PrintArgs.cmake)

INCLUDE(Dart)
