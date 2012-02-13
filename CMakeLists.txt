cmake_minimum_required(VERSION 2.6)
### Need to set rc ccompiler before enable language
if(WIN32)
    SET(CMAKE_RC_COMPILER "windres")
endif(WIN32)
project(SimGrid CXX C)

set(CMAKE_C_FLAGS "" CACHE TYPE INTERNAL FORCE)
set(CMAKE_CXX_FLAGS "" CACHE TYPE INTERNAL FORCE)
set(CMAKE_EXE_LINKER_FLAGS "" CACHE TYPE INTERNAL FORCE)

## Mapping version number -> version name
# 3.5.99 -> alpha1 (oops)
# 3.5.9{1,2} -> beta{1,2}
# 3.5.9{3,4,5} -> rc{1,2,3}
# 3.6.{0,1,2} -> release 3.6, 3.6.1, 3.6.2
# 3.7.0

set(SIMGRID_VERSION_MAJOR "3")
set(SIMGRID_VERSION_MINOR "7")
set(SIMGRID_VERSION_PATCH "0")

if(${SIMGRID_VERSION_PATCH} EQUAL "0")
	set(release_version "${SIMGRID_VERSION_MAJOR}.${SIMGRID_VERSION_MINOR}")
else(${SIMGRID_VERSION_PATCH} EQUAL "0")
	set(release_version "${SIMGRID_VERSION_MAJOR}.${SIMGRID_VERSION_MINOR}.${SIMGRID_VERSION_PATCH}")
endif(${SIMGRID_VERSION_PATCH} EQUAL "0")
set(libsimgrid_version "${release_version}")
set(libgras_version "${release_version}")
set(libsmpi_version "${release_version}")
set(GCC_NEED_VERSION "4.0")
set(APPLE_NEED_GCC_VERSION "4.6")

### SET THE LIBRARY EXTENSION AND GCC VERSION
if(APPLE) #MAC
	set(LIB_EXE "dylib")
else(APPLE)
    if(WIN32) #WINDOWS
        set(LIB_EXE "a")
        set(BIN_EXE ".exe")
    else(WIN32) #UNIX
	    set(LIB_EXE "so")
    endif(WIN32)
endif(APPLE)

string(REGEX MATCH "gcc" GCC "${CMAKE_C_COMPILER}")
if(GCC)
	exec_program("${CMAKE_C_COMPILER} --version" OUTPUT_VARIABLE "COMPILER_C_VERSION")
	exec_program("${CMAKE_CXX_COMPILER} --version" OUTPUT_VARIABLE "COMPILER_CXX_VERSION")
	string(REGEX MATCH "[0-9].[0-9].[0-9]" COMPILER_C_VERSION "${COMPILER_C_VERSION}")
	string(REGEX MATCH "[0-9].[0-9].[0-9]" COMPILER_CXX_VERSION "${COMPILER_CXX_VERSION}")
	
	string(REGEX MATCH "^[0-9].[0-9]" COMPILER_C_VERSION_MAJOR_MINOR "${COMPILER_C_VERSION}")
	string(REPLACE "${COMPILER_C_VERSION_MAJOR_MINOR}." "" COMPILER_C_VERSION_PATCH "${COMPILER_C_VERSION}")
	
	if(${GCC_NEED_VERSION} GREATER  COMPILER_C_VERSION_MAJOR_MINOR)
	    message(FATAL_ERROR "Gcc must be to version ${GCC_NEED_VERSION} current version ${COMPILER_C_VERSION_MAJOR_MINOR}")
	endif(${GCC_NEED_VERSION} GREATER  COMPILER_C_VERSION_MAJOR_MINOR)
endif(GCC)

string(REGEX MATCH "cl.exe" VBC "${CMAKE_C_COMPILER}")
if(VBC)
    message(FATAL_ERROR "VB is not yet supported by Simgrid.")
endif(VBC)

### Find programs and paths
FIND_PROGRAM(GCOV_PATH gcov)
include(FindPerl)
if(NOT PERL_EXECUTABLE)
	message(FATAL_ERROR "-- SimGrid cannot be compiled without Perl installed -- sorry. Bailling out.")
endif(NOT PERL_EXECUTABLE)

if(CMAKE_CACHE_MAJOR_VERSION EQUAL "2" AND CMAKE_CACHE_MINOR_VERSION GREATER "7") #need cmake version 2.8
	get_filename_component(TRUE_CMAKE_HOME_DIRECTORY "${CMAKE_HOME_DIRECTORY}" REALPATH)
	get_filename_component(TRUE_CMAKE_BINARY_DIR "${CMAKE_BINARY_DIR}" REALPATH)
	if(${TRUE_CMAKE_HOME_DIRECTORY} STREQUAL ${CMAKE_HOME_DIRECTORY})
	else(${TRUE_CMAKE_HOME_DIRECTORY} STREQUAL ${CMAKE_HOME_DIRECTORY})
		message(STATUS "Change link path '${CMAKE_HOME_DIRECTORY}' to true path '${TRUE_CMAKE_HOME_DIRECTORY}'")
		set(CMAKE_HOME_DIRECTORY "${TRUE_CMAKE_HOME_DIRECTORY}" CACHE TYPE INTERNAL FORCE)
		set(CMAKE_SOURCE_DIR "${TRUE_CMAKE_HOME_DIRECTORY}" CACHE TYPE INTERNAL FORCE)
	endif(${TRUE_CMAKE_HOME_DIRECTORY} STREQUAL ${CMAKE_HOME_DIRECTORY})
	
	if(${TRUE_CMAKE_BINARY_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	else(${TRUE_CMAKE_BINARY_DIR} STREQUAL ${CMAKE_BINARY_DIR})
		message(STATUS "Change link path '${CMAKE_BINARY_DIR}' to true path '${TRUE_CMAKE_BINARY_DIR}'")
		set(CMAKE_BINARY_DIR "${TRUE_CMAKE_BINARY_DIR}" CACHE TYPE INTERNAL FORCE)
	endif(${TRUE_CMAKE_BINARY_DIR} STREQUAL ${CMAKE_BINARY_DIR})	
endif(CMAKE_CACHE_MAJOR_VERSION EQUAL "2" AND CMAKE_CACHE_MINOR_VERSION GREATER "7")

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
CHECK_TYPE_SIZE("void*" SIZEOF_VOIDSTAR)
IF(SIZEOF_VOIDSTAR EQUAL 4)
  SET(ARCH_32_BITS 1)
ELSE(SIZEOF_VOIDSTAR EQUAL 4)
  SET(ARCH_32_BITS 0)
ENDIF(SIZEOF_VOIDSTAR EQUAL 4)

if(WIN32)

    #Need env INCLUDE 
    set(CMAKE_INCLUDE_WIN "${CMAKE_C_COMPILER}")
    set(CMAKE_LIB_WIN "${CMAKE_C_COMPILER}")
    string(REGEX REPLACE "/bin/gcc.*" "/include"  CMAKE_INCLUDE_WIN "${CMAKE_INCLUDE_WIN}")
    string(REGEX REPLACE "/bin/gcc.*" "/lib"  CMAKE_LIB_WIN "${CMAKE_LIB_WIN}")
    set(INCLUDES ${INCLUDES} ${CMAKE_INCLUDE_WIN})
        
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
    else(CMAKE_COMPILER_IS_GNUCC)
        message(FATAL_ERROR "Please use MinGW to compile SimGrid!")
    endif(CMAKE_COMPILER_IS_GNUCC)
    
    if(ARCH_32_BITS)	### Arch 32bits
        set(_WIN32 1)
    else(ARCH_32_BITS)	### Arch 64bits
        set(_WIN64 1)
    endif(ARCH_32_BITS)

    set(NSIS_WIN_VERSION $ENV{PROCESSOR_ARCHITEW6432})
    string(TOLOWER ${NSIS_WIN_VERSION} NSIS_WIN_VERSION)
    
    set(_XBT_WIN32 1)
    
    message(STATUS "C_COMPILER			${CMAKE_C_COMPILER} ${COMPILER_C_VERSION}")
    message(STATUS "CXX_COMPILER			${CMAKE_CXX_COMPILER} ${COMPILER_CXX_VERSION}")
    message(STATUS "CMAKE_RC_COMPILER	        ${CMAKE_RC_COMPILER}")     
    message(STATUS "INCLUDE			${CMAKE_INCLUDE_WIN}")
    message(STATUS "LIB				${CMAKE_LIB_WIN}")
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

### Make tests
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/AddTests.cmake)
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/CTestConfig.cmake)

### Setup the distrib
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/Distrib.cmake)

### Pipol compilation
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/Pipol.cmake)

### Build the doc
if(NOT WIN32)
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/GenerateDoc.cmake)
else(NOT WIN32)
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/GenerateDocWin.cmake)  
endif(NOT WIN32)

### Print ARGS
include(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/PrintArgs.cmake)

INCLUDE(Dart)
