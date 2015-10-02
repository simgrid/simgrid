cmake_minimum_required(VERSION 2.6)
# Java requires 2.8.6
message(STATUS "Cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")

project(SimGrid C CXX)

#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#
#     Check for the compiler        #
#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#

### Need to set rc ccompiler before enable language
if(WIN32)
  SET(CMAKE_RC_COMPILER "windres")
endif()

## 
## Check the C/C++ standard that we need
##   See also tools/cmake/GCCFlags.cmake that sets our paranoid warning flags
if (MSVC)
  message("-- You are compiling SimGrid with MicroSoft Visual C. Good luck.")

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_SCL_SECURE_NO_WARNINGS")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SCL_SECURE_NO_WARNINGS")
else() # gcc or clang
  INCLUDE(CheckCCompilerFlag)
  CHECK_C_COMPILER_FLAG(-fstack-cleaner HAVE_C_STACK_CLEANER)

  ## Request full debugging flags
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3")
  set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -g")

  if (CMAKE_COMPILER_IS_GNUCC)
    if (COMPILER_CXX_VERSION_MAJOR_MINOR STRLESS "4.7")
      message(FATAL_ERROR
              "SimGrid needs at least g++ version 4.7 to compile "
  	      "(c++11 support of previous versions is too limited).")
    endif()
  endif()

  ## We need a decent support of the c++11 standard
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("-std=gnu++11" COMPILER_SUPPORTS_CXX11)
  if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
  else() 
    message(FATAL_ERROR 
            "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. "
            "Please use a decent C++ compiler.")
  endif()

  ### And we need C11 standard, too
  include(CheckCCompilerFlag)
  CHECK_C_COMPILER_FLAG("-std=gnu11" COMPILER_SUPPORTS_C11)
  if(COMPILER_SUPPORTS_C11)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
  else()
    message(FATAL_ERROR 
            "The compiler ${CMAKE_C_COMPILER} has no C11 support. "
            "Please use a decent C compiler "
            "(note that c++11 support of ${CMAKE_CXX_COMPILER} seems ok).")
  endif()
  if(APPLE AND ("4.6" GREATER COMPILER_C_VERSION_MAJOR_MINOR))
      ### gcc 4.[1-5] cannot compile ucontext on OSX
      message(STATUS "Ucontext can't be used with this version of gcc (must be greater than 4.5)")
      set(HAVE_UCONTEXT_H 0)
  endif()
endif() # NOT MSVC

### SMPI vs. Fortran
if ((NOT DEFINED enable_smpi OR enable_smpi) AND NOT APPLE) # smpi is enabled by default
  # Call enable_language(Fortran) in order to load the build rules for
  # this language, needed by teshsuite/smpi/mpich-test/.  Use
  # CMAKE_FORCE_Fortran_COMPILER to bypass checks for a working
  # compiler (smpiff don't exist at configure time).
  include(CMakeForceCompiler)
  if(NOT COMMAND CMAKE_FORCE_Fortran_COMPILER)
    MACRO(CMAKE_FORCE_Fortran_COMPILER compiler id)
      SET(CMAKE_Fortran_COMPILER "${compiler}")
      SET(CMAKE_Fortran_COMPILER_ID_RUN TRUE)
      SET(CMAKE_Fortran_COMPILER_ID ${id})
      SET(CMAKE_Fortran_COMPILER_WORKS TRUE)
      SET(CMAKE_Fortran_COMPILER_FORCED TRUE)

      # Set old compiler id variables.
      IF("${CMAKE_Fortran_COMPILER_ID}" MATCHES "GNU")
        SET(CMAKE_COMPILER_IS_GNUG77 1)
      ENDIF("${CMAKE_Fortran_COMPILER_ID}" MATCHES "GNU")
    ENDMACRO(CMAKE_FORCE_Fortran_COMPILER)
  endif()
  CMAKE_FORCE_Fortran_COMPILER(smpiff smpiff)
  enable_language(Fortran OPTIONAL)
endif()

#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#
#     Build the version number      #
#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#

set(SIMGRID_VERSION_MAJOR "3")
set(SIMGRID_VERSION_MINOR "12")
set(SIMGRID_VERSION_PATCH "0")
set(SIMGRID_VERSION_EXTRA "-devel") # Extra words to add to version string (e.g. -rc1)

set(SIMGRID_VERSION_DATE  "2015") # Year for copyright information

if(${SIMGRID_VERSION_PATCH} EQUAL "0")
  set(release_version "${SIMGRID_VERSION_MAJOR}.${SIMGRID_VERSION_MINOR}")
else()
  set(release_version "${SIMGRID_VERSION_MAJOR}.${SIMGRID_VERSION_MINOR}.${SIMGRID_VERSION_PATCH}")
endif()

set(SIMGRID_VERSION_STRING "SimGrid version ${release_version}${SIMGRID_VERSION_EXTRA}")
set(SIMGRID_VERSION_BANNER
  "SIMGRID_VERSION_STRING\\nCopyright (c) 2004-${SIMGRID_VERSION_DATE}. The Simgrid Team.")

set(libsimgrid_version "${release_version}")
set(libsimgrid-java_version "${release_version}")

### SET THE LIBRARY EXTENSION 
if(APPLE) #MAC
  set(LIB_EXE "dylib")
else()
  if(WIN32) #WINDOWS
    set(LIB_EXE "a")
    set(BIN_EXE ".exe")
  else() #UNIX
    set(LIB_EXE "so")
  endif()
endif()

if(${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
  exec_program("${CMAKE_C_COMPILER} --version" OUTPUT_VARIABLE "COMPILER_C_VERSION")
  exec_program("${CMAKE_CXX_COMPILER} --version" OUTPUT_VARIABLE "COMPILER_CXX_VERSION")
  string(REGEX MATCH "[0-9].[0-9].[0-9]" COMPILER_C_VERSION "${COMPILER_C_VERSION}")
  string(REGEX MATCH "[0-9].[0-9].[0-9]" COMPILER_CXX_VERSION "${COMPILER_CXX_VERSION}")

  string(REGEX MATCH "^[0-9].[0-9]" COMPILER_C_VERSION_MAJOR_MINOR "${COMPILER_C_VERSION}")
  string(REPLACE "${COMPILER_C_VERSION_MAJOR_MINOR}." "" COMPILER_C_VERSION_PATCH "${COMPILER_C_VERSION}")  
endif()

exec_program("${CMAKE_LINKER} --version" OUTPUT_VARIABLE "LINKER_VERSION")
string(REGEX MATCH "[0-9].[0-9]*" LINKER_VERSION "${LINKER_VERSION}")

### Find programs and paths
FIND_PROGRAM(GCOV_PATH gcov)
include(FindPerl)
if(NOT PERL_EXECUTABLE)
  message(FATAL_ERROR "-- SimGrid cannot be compiled without Perl installed -- sorry. Bailling out.")
endif()

if (APPLE)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

### Set some variables for Cmake
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

set(INCLUDES
  ${CMAKE_HOME_DIRECTORY}
  ${CMAKE_HOME_DIRECTORY}/include
  ${CMAKE_HOME_DIRECTORY}/src
  ${CMAKE_HOME_DIRECTORY}/src/include
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/include
  ${CMAKE_BINARY_DIR}/src
  )

if(WIN32)
  set(INCLUDES ${INCLUDES} ${CMAKE_HOME_DIRECTORY}/include/xbt ${CMAKE_HOME_DIRECTORY}/src/xbt) #for win32_ucontext.[ch]
endif()

if(NOT CMAKE_CROSSCOMPILING AND EXISTS /usr/include/)
  set(INCLUDES ${INCLUDES} /usr/include/)
endif()

### Check 32bits or 64bits
INCLUDE (CheckTypeSize)
CHECK_TYPE_SIZE("int" SIZEOF_INT)
CHECK_TYPE_SIZE("long" SIZEOF_LONG)
CHECK_TYPE_SIZE("long long" SIZEOF_LONGLONG)
CHECK_TYPE_SIZE("void*" SIZEOF_VOIDSTAR)
message ("   Data model: (int)=${SIZEOF_INT} (long)=${SIZEOF_LONG} (long long)=${SIZEOF_LONGLONG} (void*)=${SIZEOF_VOIDSTAR}")
IF(SIZEOF_VOIDSTAR EQUAL 4)
  SET(ARCH_32_BITS 1)
ELSE()
  SET(ARCH_32_BITS 0)
ENDIF()

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
    endif()
    if(COMPILER_C_MINOR_VERSION)
      # set(__GNUC_MINOR__ ${COMPILER_C_MINOR_VERSION})
    endif()
  endif()

  set(NSIS_WIN_VERSION $ENV{PROCESSOR_ARCHITEW6432})
  if(NSIS_WIN_VERSION MATCHES "")
    set(NSIS_WIN_VERSION $ENV{PROCESSOR_ARCHITECTURE})
  endif()
  if(${NSIS_WIN_VERSION})
    string(TOLOWER ${NSIS_WIN_VERSION} NSIS_WIN_VERSION)
  endif()

  set(_XBT_WIN32 1)

  message(STATUS "C_COMPILER                    ${CMAKE_C_COMPILER} ${COMPILER_C_VERSION_MAJOR_MINOR}")
  message(STATUS "CXX_COMPILER                  ${CMAKE_CXX_COMPILER} ${COMPILER_CXX_VERSION_MAJOR_MINOR}")
  message(STATUS "CMAKE_RC_COMPILER             ${CMAKE_RC_COMPILER}")
  message(STATUS "INCLUDE                       ${CMAKE_INCLUDE_WIN}")
  message(STATUS "LIB                           ${CMAKE_LIB_WIN}")
  message(STATUS "MAKE_PROGRAM                  ${CMAKE_MAKE_PROGRAM}")
  message(STATUS "CMAKE_BUILD_TOOL              ${CMAKE_BUILD_TOOL}")
  message(STATUS "LINKER                        ${CMAKE_LINKER}")
  message(STATUS "CMAKE_GENERATOR               ${CMAKE_GENERATOR}")
  message(STATUS "GNUC                          ${CMAKE_COMPILER_IS_GNUCC}")

endif()

include_directories(${INCLUDES})

### Setup Options
include(${CMAKE_HOME_DIRECTORY}/tools/cmake/Option.cmake)

### Make the *.h files with *.h.in files
include(${CMAKE_HOME_DIRECTORY}/tools/cmake/CompleteInFiles.cmake)

### Define source packages for Libs
include(${CMAKE_HOME_DIRECTORY}/tools/cmake/DefinePackages.cmake)

### Build some Maintainer files
include(${CMAKE_HOME_DIRECTORY}/tools/cmake/MaintainerMode.cmake)
include(${CMAKE_HOME_DIRECTORY}/tools/cmake/UnitTesting.cmake)

### Setup gcc & clang flags
if (NOT MSVC)
  include(${CMAKE_HOME_DIRECTORY}/tools/cmake/GCCFlags.cmake)
endif()

### Make Libs
if(NOT WIN32)
  include(${CMAKE_HOME_DIRECTORY}/tools/cmake/MakeLib.cmake)
else()
  include(${CMAKE_HOME_DIRECTORY}/tools/cmake/MakeLibWin.cmake)
endif()

if(enable_java)
  include(${CMAKE_HOME_DIRECTORY}/tools/cmake/Java.cmake)
endif()

### Make Exes
include(${CMAKE_HOME_DIRECTORY}/tools/cmake/MakeExe.cmake)

### Make tests
if(enable_memcheck_xml)
  set(enable_memcheck true)
endif()

INCLUDE(CTest)
ENABLE_TESTING()
if(WIN32)
  add_test(
      NAME basic-java
      COMMAND java -classpath ".;${CMAKE_HOME_DIRECTORY}\\simgrid.jar" masterslave.Masterslave ..\\platforms\\platform.xml masterslave\\masterslaveDeployment.xml
      WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}\\examples\\java
  )
else()
  include(${CMAKE_HOME_DIRECTORY}/tools/cmake/AddTests.cmake)
endif()
include(${CMAKE_HOME_DIRECTORY}/tools/cmake/CTestConfig.cmake)

### Setup the distrib
include(${CMAKE_HOME_DIRECTORY}/tools/cmake/Distrib.cmake)

### Build the doc (Windows downloads the doc instead of regenerating)
#
if(NOT WIN32)
  include(${CMAKE_HOME_DIRECTORY}/tools/cmake/Documentation.cmake)
else()
  find_program(WGET_PROGRAM  NAMES wget)
  message(STATUS "wget: ${WGET_PROGRAM}")
  if(WGET_PROGRAM)
    ADD_CUSTOM_TARGET(simgrid_documentation
      COMMENT "Downloading the SimGrid documentation..."
      COMMAND ${WGET_PROGRAM} -r -np -nH -nd http://simgrid.gforge.inria.fr/simgrid/${release_version}/doc/
      WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/html
    )
  endif()
endif()

### Print ARGS
include(${CMAKE_HOME_DIRECTORY}/tools/cmake/PrintArgs.cmake)

INCLUDE(Dart)
