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

### SET THE PROJECT_DIRECTORY
SET(PROJECT_DIRECTORY "${CMAKE_HOME_DIRECTORY}")

### Set some variables for Cmake
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(INCLUDES "-I${PROJECT_DIRECTORY} -I${PROJECT_DIRECTORY}/include -I${PROJECT_DIRECTORY}/src/include -I${PROJECT_DIRECTORY}/src -I${CMAKE_CURRENT_BINARY_DIR}/src")
if(WIN32)
    set(INCLUDES "${INCLUDES} -I${PROJECT_DIRECTORY}/include/xbt -I${PROJECT_DIRECTORY}/src/xbt") #for win32_ucontext.[ch]
endif(WIN32)
set(CMAKE_SOURCE_DIR ${PROJECT_SOURCE_DIRECTORY})

if(EXISTS /usr/include/)
	set(INCLUDES "${INCLUDES} -I/usr/include/")
endif(EXISTS /usr/include/)

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
	            set(INCLUDES "${INCLUDES} -I\"${include_dir}\"")
	        else(testoki)
	            set(INCLUDES "${INCLUDES} -I${include_dir}")
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
    
    message("Be sure all those variables are correctely set.")
    message("	-- C_COMPILER			$ENV{CC}")
    message("	-- CXX_COMPILER			$ENV{CXX}")    
    message("	-- INCLUDE			$ENV{INCLUDE}")
    message("	-- LIB				$ENV{LIB}")
    message("	-- MAKE_PROGRAM			${CMAKE_MAKE_PROGRAM}")
    message("	-- CMAKE_BUILD_TOOL		${CMAKE_BUILD_TOOL}")
    message("	-- LINKER			${CMAKE_LINKER}")
    message("	-- CMAKE_GENERATOR		${CMAKE_GENERATOR}")
    message("	-- INCLUDES			${INCLUDES}")
    message("	-- BORLAND			${BORLAND}")
    message("	-- VISUALC			${MSVC}")
    message("	-- GNUC			        ${CMAKE_COMPILER_IS_GNUCC}\n\n")
    
endif(WIN32)

add_definitions (${INCLUDES})

### Setup Options
include(${PROJECT_DIRECTORY}/buildtools/Cmake/Option.cmake)

### Make the *.h files with *.h.in files
include(${PROJECT_DIRECTORY}/buildtools/Cmake/CompleteInFiles.cmake)

### Define source packages for Libs 
include(${PROJECT_DIRECTORY}/buildtools/Cmake/DefinePackages.cmake)

### Build some Maintainer files
include(${PROJECT_DIRECTORY}/buildtools/Cmake/MaintainerMode.cmake)

### Setup gcc flags
include(${PROJECT_DIRECTORY}/buildtools/Cmake/Flags.cmake)

### Make Libs and Exes
if(WIN32)
    include(${PROJECT_DIRECTORY}/buildtools/Cmake/MakeExeLibWin.cmake)
else(WIN32)
    include(${PROJECT_DIRECTORY}/buildtools/Cmake/MakeExeLib.cmake)
endif(WIN32)

### Build the doc
if(enable_doc)
	ADD_CUSTOM_COMMAND(
	  OUTPUT ${PROJECT_DIRECTORY}/doc/html
	  DEPENDS 	${PROJECT_DIRECTORY}/doc/FAQ.doc 
	  			${PROJECT_DIRECTORY}/buildtools/Cmake/GenerateDoc.cmake 
	  			${PROJECT_DIRECTORY}/buildtools/Cmake/doc/CMakeLists.txt
	  COMMAND ${CMAKE_MAKE_PROGRAM} html
	  COMMENT "Generating the simgrid documentation..."
	)
	ADD_CUSTOM_TARGET(simgrid_documentation ALL
	                  DEPENDS ${PROJECT_DIRECTORY}/doc/html)
endif(enable_doc)

### Make Simgrid.jar and Java examples
if(HAVE_JAVA)
	include(${PROJECT_DIRECTORY}/buildtools/Cmake/MakeJava.cmake)
endif(HAVE_JAVA)

### Make tests
if(NOT WIN32)
include(${PROJECT_DIRECTORY}/buildtools/Cmake/AddTests.cmake)
endif(NOT WIN32)

include(${PROJECT_DIRECTORY}/buildtools/Cmake/CTestConfig.cmake)

### Setup the distrib
include(${PROJECT_DIRECTORY}/buildtools/Cmake/Distrib.cmake)

### Pipol compilation
include(${PROJECT_DIRECTORY}/buildtools/Cmake/Pipol.cmake)

### Print ARGS
include(${PROJECT_DIRECTORY}/buildtools/Cmake/PrintArgs.cmake)

INCLUDE(Dart)