# Build the version number

set(SIMGRID_VERSION_MAJOR "3")
set(SIMGRID_VERSION_MINOR "26")
set(SIMGRID_VERSION_PATCH "0") # odd => git branch; even => stable release or released snapshot

if(${SIMGRID_VERSION_PATCH} EQUAL "0")
  set(release_version "${SIMGRID_VERSION_MAJOR}.${SIMGRID_VERSION_MINOR}")
else()
  set(release_version "${SIMGRID_VERSION_MAJOR}.${SIMGRID_VERSION_MINOR}.${SIMGRID_VERSION_PATCH}")
endif()

message(STATUS "Configuring SimGrid v${release_version}")

set(SIMGRID_VERSION_STRING "SimGrid version ${release_version}")

set(libsimgrid_version "${release_version}")
set(libsimgrid-java_version "${release_version}")

# Basic checks on cmake
cmake_minimum_required(VERSION 3.5)
#for lto, to avoid warning (should be removed when switching to requiring cmake >= 3.9)
if(NOT CMAKE_VERSION VERSION_LESS "3.9")
  cmake_policy(SET CMP0069 NEW)
endif()
message(STATUS "Cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_HOME_DIRECTORY}/tools/cmake/Modules)

project(simgrid C CXX)

# customizable installation directories
include(GNUInstallDirs)

## Save compiler flags preset with environment variables CFLAGS or CXXFLAGS;
## they will used within smpicc, smpicxx.
## Do it early so that we get their genuine values. The same will be done later for Fortran.
string(REGEX REPLACE " *-f[a-z]+-prefix-map=[^ ]*" "" SMPI_C_FLAGS "${CMAKE_C_FLAGS}")
string(REGEX REPLACE " *-f[a-z]+-prefix-map=[^ ]*" "" SMPI_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#
#     Check for the compiler        #
#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#
## 
## Check the C/C++ standard that we need
##   See also tools/cmake/Flags.cmake that sets our paranoid warning flags
INCLUDE(CheckCCompilerFlag)
CHECK_C_COMPILER_FLAG(-fstack-cleaner HAVE_C_STACK_CLEANER)

## Request full debugging flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3")

if (CMAKE_COMPILER_IS_GNUCC)    
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.0")
    message(FATAL_ERROR
            "SimGrid needs at least g++ version 5.0 to compile but you have ${CMAKE_CXX_COMPILER_VERSION}."
            "You need a sufficient support of c++14 to compile SimGrid.")
  endif()
endif()

## We need a decent support of the C++14 and C11 standards
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if (CMAKE_C_COMPILER_ID STREQUAL "Intel" AND CMAKE_C11_EXTENSION_COMPILE_OPTION STREQUAL "-std=c11")
  set(CMAKE_C11_EXTENSION_COMPILE_OPTION "-std=gnu11")
endif()

### Check threading support
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads)

### Setup Options
include(${CMAKE_HOME_DIRECTORY}/tools/cmake/Option.cmake)

### SMPI vs. Fortran
if ((NOT DEFINED enable_smpi) OR enable_smpi) 
  # First unset the compiler in case we're re-running cmake over a previous
  # configuration where it was saved as smpiff
  unset(CMAKE_Fortran_COMPILER)
  
  SET(SMPI_FORTRAN 0)
  if(enable_fortran)
    enable_language(Fortran OPTIONAL)
  endif()
  
  if(CMAKE_Fortran_COMPILER)

    # Fortran compiler detected: save it, then replace by smpiff
    set(SMPI_Fortran_COMPILER "${CMAKE_Fortran_COMPILER}" CACHE FILEPATH "The real Fortran compiler")

	# Set flags/libs to be used in smpiff
    if(CMAKE_Fortran_COMPILER_ID MATCHES "GNU")
      set(SMPI_Fortran_FLAGS "\"-fpic\" \"-ff2c\" \"-fno-second-underscore\"")
      set(SMPI_Fortran_LIBS "\"-lgfortran\"")
      set(SMPI_GFORTRAN 1)
    elseif(CMAKE_Fortran_COMPILER_ID MATCHES "Intel")
      set(SMPI_Fortran_FLAGS "\"-fPIC\" \"-nofor-main\"")
      set(SMPI_Fortran_LIBS "\"-lifcore\"")
      set(SMPI_IFORT 1)
    elseif(CMAKE_Fortran_COMPILER_ID MATCHES "PGI|Flang") # flang
      set(SMPI_Fortran_FLAGS "\"-fPIC\"")
      set(SMPI_Fortran_LIBS "")
      set(SMPI_FLANG 1)
    endif()
    string(REGEX REPLACE " *-f[a-z]+-prefix-map=[^ ]*" "" SMPI_Fortran_FLAGS "${SMPI_Fortran_FLAGS} ${CMAKE_Fortran_FLAGS}")

    ## Request debugging flags for Fortran too
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -g")

    set(SMPI_FORTRAN 1)
  endif(CMAKE_Fortran_COMPILER)

endif()

### SET THE LIBRARY EXTENSION 
if(APPLE)
  set(LIB_EXE "dylib")
elseif(WIN32)
  set(LIB_EXE "a")
  set(BIN_EXE ".exe")
else()
  set(LIB_EXE "so")
endif()

execute_process(COMMAND ${CMAKE_LINKER} -v OUTPUT_VARIABLE LINKER_VERSION ERROR_VARIABLE LINKER_VERSION)
string(REGEX MATCH "[0-9].[0-9]*" LINKER_VERSION "${LINKER_VERSION}")

### Find programs and paths
FIND_PROGRAM(GCOV_PATH gcov)
include(FindPerl)
if(NOT PERL_FOUND)
  message(FATAL_ERROR "Please install Perl to compile SimGrid.")
endif()

# tesh.py needs python 3 (or the module python-subprocess32 on python2.8+)
if(CMAKE_VERSION VERSION_LESS "3.12")
  set(PythonInterp_FIND_VERSION 3)
  set(PythonInterp_FIND_VERSION_COUNT 1)
  set(PythonInterp_FIND_VERSION_MAJOR 3)
  include(FindPythonInterp)
  if(NOT PYTHONINTERP_FOUND)
    message(FATAL_ERROR "Please install Python (version 3 or higher) to compile SimGrid.")
  endif()
else()
  find_package(Python3 COMPONENTS Interpreter Development)
  if(NOT Python3_Interpreter_FOUND)
    message(FATAL_ERROR "Please install Python (version 3 or higher) to compile SimGrid.")
  endif()
  set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
endif()

SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

### Compute the include paths

# Only include public headers by default
include_directories(
   ${CMAKE_BINARY_DIR}/include
   ${CMAKE_HOME_DIRECTORY}/include
)

# Compute the ones that should be added when compiling the library
set(INTERNAL_INCLUDES
  ${CMAKE_BINARY_DIR}
  ${CMAKE_HOME_DIRECTORY}
  ${CMAKE_HOME_DIRECTORY}/src/include
  )

if(enable_smpi)
  set (INTERNAL_INCLUDES ${INTERNAL_INCLUDES} ${CMAKE_HOME_DIRECTORY}/src/smpi/include)
endif()

if(NOT CMAKE_CROSSCOMPILING AND EXISTS /usr/include/)
  set(INTERNAL_INCLUDES ${INTERNAL_INCLUDES} /usr/include/)
endif()

if(WIN32)
  set(CMAKE_INCLUDE_WIN "${CMAKE_C_COMPILER}")
  set(CMAKE_LIB_WIN "${CMAKE_C_COMPILER}")
  string(REGEX REPLACE "/bin/gcc.*" "/include"  CMAKE_INCLUDE_WIN "${CMAKE_INCLUDE_WIN}")
  string(REGEX REPLACE "/bin/gcc.*" "/lib"  CMAKE_LIB_WIN "${CMAKE_LIB_WIN}")
  set(INTERNAL_INCLUDES ${INTERNAL_INCLUDES} ${CMAKE_INCLUDE_WIN})
  unset(CMAKE_INCLUDE_WIN)
endif()

# library dependency cannot start with a space (CMP0004), so initialize it with something that is never deactivated.
set(SIMGRID_DEP "-lm") 

### Determine the assembly flavor that we need today
set(HAVE_RAW_CONTEXTS 0)
include(CMakeDetermineSystem)
IF(CMAKE_SYSTEM_PROCESSOR MATCHES ".86|AMD64|amd64")
  IF(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
    message(STATUS "System processor: i686 (${CMAKE_SYSTEM_PROCESSOR}, 32 bits)")
    set(SIMGRID_PROCESSOR_i686 1)
    set(SIMGRID_PROCESSOR_x86_64 0)
  ELSE()
    message(STATUS "System processor: x86_64 (${CMAKE_SYSTEM_PROCESSOR}, 64 bits)")
    set(SIMGRID_PROCESSOR_i686 0)
    set(SIMGRID_PROCESSOR_x86_64 1)
  ENDIF()
  if (WIN32)
    message(STATUS "Disable fast raw contexts on Windows.")
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4 AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    message(STATUS "Disable fast raw contexts on x32 ABI.")
  else()
    set(HAVE_RAW_CONTEXTS 1)
  endif()
ELSE()
  set(SIMGRID_PROCESSOR_i686 0)
  set(SIMGRID_PROCESSOR_x86_64 0)
ENDIF()

include(CheckFunctionExists)
include(CheckTypeSize)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckSymbolExists)

set(HAVE_GRAPHVIZ 0)
set(SIMGRID_HAVE_LUA 0)
if(minimal-bindings)
  message(STATUS "Don't even look for graphviz nor lua, as we build minimal binding libraries.")
else()
  include(FindGraphviz)
  if(enable_lua)
    include(FindLuaSimgrid)
  endif()
endif()

set(SIMGRID_HAVE_NS3 0)
if(enable_ns3)
  include(FindNS3)
  if (SIMGRID_HAVE_NS3)
    set(SIMGRID_HAVE_NS3 1)
    foreach(lib core csma point-to-point internet network applications wifi)
      set(SIMGRID_DEP "${SIMGRID_DEP} -lns${NS3_VERSION}-${lib}${NS3_SUFFIX}")
    endforeach()
  else()
    message(FATAL_ERROR "Cannot find ns-3. Please install it (apt-get install ns3 libns3-dev) or disable that cmake option")
  endif()
endif()

set(SIMGRID_HAVE_MSG 0)
if(enable_msg)
  set(SIMGRID_HAVE_MSG 1)
endif()

if(WIN32)
  set(Boost_USE_STATIC_LIBS 1)
endif()

set(HAVE_PAPI 0)
if(enable_smpi_papi)
  include(FindPAPI)
  if (NOT HAVE_PAPI)
    message(FATAL_ERROR "Cannot find PAPI. Please install it (apt-get install papi-tools libpapi-dev) or disable PAPI bindings.")
  endif()
endif()

# But we do need the core of Boost
# cmake before 3.13.1 does not know about stacktrace components. Fix it.
# Usable components: https://www.boost.org/doc/libs/1_65_1/doc/html/stacktrace/configuration_and_build.html
set(_Boost_STACKTRACE_HEADERS           "boost/stacktrace.hpp")
set(_Boost_STACKTRACE_BACKTRACE_HEADERS "boost/stacktrace.hpp")
set(_Boost_STACKTRACE_ADDR2LINE_HEADERS "boost/stacktrace.hpp")

  if(minimal-bindings) # When we want a minimal jarfile, don't even search for boost optional components
    message(STATUS "Don't even look for boost optional components, as we build minimal binding libraries.")
    find_package(Boost 1.48)
  else()
    find_package(Boost 1.59 OPTIONAL_COMPONENTS context stacktrace_backtrace stacktrace_addr2line)
  endif()
  if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    message(STATUS "Mandatory components found. SimGrid is compilable.")
    if (NOT minimal-bindings)
      message(STATUS "Looking for optional Boost components:")
      set(Boost_FOUND 1) # These components are optionals
      CHECK_INCLUDE_FILE("backtrace.h" HAVE_BACKTRACE_H) # check that backtrace is actually possible
      if (Boost_STACKTRACE_BACKTRACE_FOUND AND HAVE_BACKTRACE_H)
        message (STATUS "  stacktrace: found the fast 'backtrace' implementation. Activating human-readable stack traces.")
        set(HAVE_BOOST_STACKTRACE_BACKTRACE 1)
      else()
        set(HAVE_BOOST_STACKTRACE_BACKTRACE 0)
        if (Boost_STACKTRACE_ADDR2LINE_FOUND)
          message (STATUS "  stacktrace: found the slow 'addr2line' implementation. Activating human-readable stack traces.")
          set(HAVE_BOOST_STACKTRACE_ADDR2LINE 1)
        else()
          message (STATUS "  stacktrace: MISSING. Install libboost-stacktrace-dev to display the stacktraces.")
          set(HAVE_BOOST_STACKTRACE_ADDR2LINE 0)
        endif()
      endif()

      if(Boost_CONTEXT_FOUND)
        message (STATUS "  context: found. Activating Boost contexts.")
        set(HAVE_BOOST_CONTEXTS 1)
      else()
        message (STATUS "  context: MISSING. Install libboost-context-dev for this optional feature.")
        set(HAVE_BOOST_CONTEXTS 0)
      endif()
    endif()
  else()
    if(APPLE)
      message(FATAL_ERROR "Boost libraries not found. Try to install them with 'sudo fink install boost1.53.nopython' (check the exact name with 'fink list boost') or 'sudo brew install boost'")
    else()
      find_package(Boost 1.48) #try without optional libraries
      if(NOT Boost_FOUND)
        message(FATAL_ERROR "Boost libraries not found. Install libboost-dev (>= 1.48.0).")
      else()
        include_directories(${Boost_INCLUDE_DIRS})
        message(STATUS "Mandatory components found. SimGrid is compilable.")
      endif()
    endif()
  endif() 

# Checks for header libraries functions.
CHECK_LIBRARY_EXISTS(rt      clock_gettime           "" HAVE_POSIX_GETTIME)
CHECK_LIBRARY_EXISTS(pthread pthread_setaffinity_np  "" HAVE_PTHREAD_SETAFFINITY)
CHECK_INCLUDE_FILE("pthread_np.h" HAVE_PTHREAD_NP_H) # for pthread_setaffinity_np() on FreeBSD

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(CMAKE_REQUIRED_DEFINITIONS "-D_XOPEN_SOURCE=700 -D_DARWIN_C_SOURCE")
elseif(MINGW)
  # Use the GNU version of unusual modifiers like PRIx64
  add_definitions(-D__USE_MINGW_ANSI_STDIO=1)
  set(CMAKE_REQUIRED_DEFINITIONS "-D__USE_MINGW_ANSI_STDIO=1")
else()
  set(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
endif()

CHECK_INCLUDE_FILE("valgrind/valgrind.h" HAVE_VALGRIND_H)
CHECK_INCLUDE_FILE("unistd.h" HAVE_UNISTD_H)
CHECK_INCLUDE_FILE("signal.h" HAVE_SIGNAL_H)
CHECK_INCLUDE_FILE("sys/param.h" HAVE_SYS_PARAM_H)
CHECK_INCLUDE_FILE("sys/sysctl.h" HAVE_SYS_SYSCTL_H)
CHECK_INCLUDE_FILE("linux/futex.h" HAVE_FUTEX_H)

CHECK_FUNCTION_EXISTS(dlfunc HAVE_DLFUNC)
CHECK_FUNCTION_EXISTS(gettimeofday HAVE_GETTIMEOFDAY)
CHECK_FUNCTION_EXISTS(nanosleep HAVE_NANOSLEEP)
CHECK_FUNCTION_EXISTS(getdtablesize HAVE_GETDTABLESIZE)
CHECK_FUNCTION_EXISTS(sysconf HAVE_SYSCONF)
CHECK_FUNCTION_EXISTS(process_vm_readv HAVE_PROCESS_VM_READV)
CHECK_FUNCTION_EXISTS(mmap HAVE_MMAP)
CHECK_FUNCTION_EXISTS(mremap HAVE_MREMAP)

CHECK_SYMBOL_EXISTS(vasprintf stdio.h HAVE_VASPRINTF)
if(MINGW)
  # The detection of vasprintf fails on MinGW, assumingly because it's
  # defined as an inline function in stdio.h instead of a regular
  # function. So force the result to be 1 despite of the test.
  set(HAVE_VASPRINTF 1)
endif()

CHECK_INCLUDE_FILE("sys/sendfile.h" HAVE_SENDFILE_H)
CHECK_FUNCTION_EXISTS(sendfile HAVE_SENDFILE)
if(HAVE_SENDFILE_H AND HAVE_SENDFILE)
  set(SG_HAVE_SENDFILE 1)
else()
  set(SG_HAVE_SENDFILE 0)
endif()

if(enable_model-checking AND NOT "${CMAKE_SYSTEM}" MATCHES "Linux|FreeBSD")
  message(WARNING "Support for model-checking has not been enabled on ${CMAKE_SYSTEM}: disabling it")
  set(enable_model-checking FALSE)
endif()

if(HAVE_MMAP)
  SET(HAVE_MMALLOC 1)
else()
  SET(HAVE_MMALLOC 0)
  if(enable_model-checking)
    message(STATUS "Warning: support for model-checking has been disabled because you are missing either mmap or __thread.")
  endif()
  SET(enable_model-checking 0)
endif()

if(enable_jedule)
  set(SIMGRID_HAVE_JEDULE 1)
else()
  set(SIMGRID_HAVE_JEDULE 0)
endif()

if(enable_mallocators)
  SET(SIMGRID_HAVE_MALLOCATOR 1)
else()
  SET(SIMGRID_HAVE_MALLOCATOR 0)
endif()

if (minimal-bindings)
  message(STATUS "Don't look for libunwind as we build minimal binding libraries.")
  if(enable_model-checking)
    message(FATAL_ERROR "You cannot enable model-checking and minimal-bindings at the same time.")
  endif()
else()
  include(FindLibunwind)
  if(HAVE_LIBUNWIND)
    SET(SIMGRID_DEP "${SIMGRID_DEP} ${LIBUNWIND_LIBRARIES}")
  else()
    if(enable_model-checking)
      message(FATAL_ERROR "Please install libunwind-dev libdw-dev libelf-dev libevent-dev if you want to compile the SimGrid model checker.")
    endif()
  endif()
endif()

if(enable_model-checking)
  find_package(Libdw REQUIRED)
  find_package(Libelf REQUIRED)
  find_package(Libevent REQUIRED)
  include_directories(${LIBDW_INCLUDE_DIR} ${LIBELF_INCLUDE_DIR} ${LIBEVENT_INCLUDE_DIR})
  set(SIMGRID_DEP "${SIMGRID_DEP} ${LIBEVENT_LIBRARIES} ${LIBELF_LIBRARIES} ${LIBDW_LIBRARIES}")
  set(SIMGRID_HAVE_MC 1)
  if("${CMAKE_SYSTEM}" MATCHES "FreeBSD" AND enable_java)
    message(WARNING "FreeBSD + Model-Checking + Java = too much for now. Disabling the Java bindings.")
    set(enable_java FALSE)
  endif()
else()
  SET(SIMGRID_HAVE_MC 0)  
  set(HAVE_MMALLOC 0)
endif()
mark_as_advanced(PATH_LIBDW_H)
mark_as_advanced(PATH_LIBDW_LIB)

if(enable_java AND NOT enable_msg)
  message(FATAL_ERROR "Cannot activate the Java bindings without the MSG module. Either add -Denable_msg=ON or -Denable_java=OFF")
endif()

if (enable_model-checking AND enable_ns3)
  message(FATAL_ERROR "Cannot activate both model-checking and ns-3 bindings: ns-3 pulls too much dependencies for the MC to work")
endif()

if(enable_smpi)
  SET(HAVE_SMPI 1)
  if(WIN32)
    message (STATUS "Warning: no support for SMPI automatic privatization on Windows.")
    SET(HAVE_PRIVATIZATION 0)
  else()
    SET(HAVE_PRIVATIZATION 1)
  endif()
else()
  SET(HAVE_SMPI 0)
endif()

#--------------------------------------------------------------------------------------------------
### Check what context backends are available

set(HAVE_UCONTEXT_CONTEXTS 0)
CHECK_INCLUDE_FILE("ucontext.h" HAVE_UCONTEXT_H)
if(NOT HAVE_UCONTEXT_H)
  message(STATUS "No ucontext factory: <ucontext.h> not found.")
elseif(APPLE)
  message(STATUS "No ucontext factory: Apple don't want us to use them.")
  set(HAVE_UCONTEXT_H 0)
else()
  try_compile(compile_makecontext ${CMAKE_BINARY_DIR} ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_makecontext.c
    OUTPUT_VARIABLE compile_makecontext_output)

  #If can have both context
  if(compile_makecontext)
    set(HAVE_UCONTEXT_CONTEXTS 1)
    message(STATUS "Support for ucontext factory ok.")
  else()
    message(STATUS "Error: <ucontext.h> exists, but makecontext is not compilable. Compilation output:\n ${compile_makecontext_output}")
    message(STATUS "No ucontext factory: makecontext() is not compilable.")
  endif()

  # Stack setup (size and address)
  try_run(RUN_makecontext_VAR COMPILE_makecontext_VAR
    ${CMAKE_BINARY_DIR} ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_stacksetup.c
    RUN_OUTPUT_VARIABLE stack_setup)

  LIST(LENGTH stack_setup stack_setup_len)
  if("${stack_setup_len}" STREQUAL "2")
    LIST(GET stack_setup 0 makecontext_addr)
    LIST(GET stack_setup 1 makecontext_size)
    set(sg_makecontext_stack_addr "#define sg_makecontext_stack_addr(skaddr) (${makecontext_addr})")
    set(sg_makecontext_stack_size "#define sg_makecontext_stack_size(sksize) (${makecontext_size})")
  else()
    message(FATAL_ERROR "Could not figure out the stack setup. Compil: ${RUN_makecontext_VAR}. Exec: ${COMPILE_makecontext_VAR}. Output: ${stack_setup}")
  endif()
endif()

# Stack growth direction (upward or downward). Used for the following contexts: SysV, raw, Boost
try_run(RUN_stackgrowth_VAR COMPILE_stackgrowth_VAR
  ${CMAKE_BINARY_DIR}
  ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_stackgrowth.c
  RUN_OUTPUT_VARIABLE stack
  COPY_FILE test_stackgrowth)

if("${stack}" STREQUAL "down")
  set(PTH_STACKGROWTH "-1")
elseif("${stack}" STREQUAL "up")
  set(PTH_STACKGROWTH "1")
else()
  if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
    set(PTH_STACKGROWTH "-1")
  elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i686")
    set(PTH_STACKGROWTH "-1")
  else()
    message(FATAL_ERROR "Could not figure out the stack direction. Test prog returned: ${stack}; CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}.")
  endif()
endif()
# If the test ran well, remove the test binary
file(REMOVE test_stackgrowth)
#--------------------------------------------------------------------------------------------------

###############
## GIT version check
##
if(EXISTS ${CMAKE_HOME_DIRECTORY}/.git/)
  execute_process(COMMAND git rev-parse --verify --short HEAD
     WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
     OUTPUT_VARIABLE GIT_VERSION
     OUTPUT_STRIP_TRAILING_WHITESPACE)
  # Check for uncommitted changes
  execute_process(COMMAND git diff --name-only HEAD
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
    OUTPUT_VARIABLE files_changed)
  if(files_changed)
    set(GIT_VERSION "${GIT_VERSION}-dirty")
  endif()
elseif(EXISTS ${CMAKE_HOME_DIRECTORY}/.gitversion)
  FILE(STRINGS ${CMAKE_HOME_DIRECTORY}/.gitversion GIT_VERSION)
else()
  set(GIT_VERSION "none, release version")
endif()
message(STATUS "Git version: ${GIT_VERSION}")

### Define source packages for Libs
include(${CMAKE_HOME_DIRECTORY}/tools/cmake/DefinePackages.cmake)

### Setup gcc & clang flags (include after DefinePackage.cmake, and before generating header files)
if (NOT MSVC)
  include(${CMAKE_HOME_DIRECTORY}/tools/cmake/Flags.cmake)
endif()

### Generate the required headers and scripts
#############################################

# Avoid triggering a (full) rebuild by touching the files if they did not really change
configure_file("${CMAKE_HOME_DIRECTORY}/src/internal_config.h.in"    "${CMAKE_BINARY_DIR}/src/internal_config.h.generated"    @ONLY IMMEDIATE)
configure_file("${CMAKE_HOME_DIRECTORY}/include/simgrid/version.h.in" "${CMAKE_BINARY_DIR}/include/simgrid/version.h.generated" @ONLY IMMEDIATE)
configure_file("${CMAKE_HOME_DIRECTORY}/include/simgrid/config.h.in" "${CMAKE_BINARY_DIR}/include/simgrid/config.h.generated" @ONLY IMMEDIATE)
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/src/internal_config.h.generated ${CMAKE_BINARY_DIR}/src/internal_config.h)
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/include/simgrid/version.h.generated ${CMAKE_BINARY_DIR}/include/simgrid/version.h)
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/include/simgrid/config.h.generated ${CMAKE_BINARY_DIR}/include/simgrid/config.h)
file(REMOVE ${CMAKE_BINARY_DIR}/src/internal_config.h.generated)
file(REMOVE ${CMAKE_BINARY_DIR}/include/simgrid/config.h.generated)
file(REMOVE ${CMAKE_BINARY_DIR}/include/simgrid/version.h.generated)

# We need two versions of the SMPI scripts because they contain the path to the library
# so, it depends of whether SimGrid is installed, or run from the sources (during the build)

file(READ ${CMAKE_HOME_DIRECTORY}/src/smpi/smpitools.sh SMPITOOLS_SH) # Definitions shared amongst all SMPI scripts, inlined in each of them

### SMPI script used when simgrid is installed
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(includedir "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
set(libdir "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set(includeflag "-I${includedir} -I${includedir}/smpi")
set(CMAKE_SMPI_COMMAND "export LD_LIBRARY_PATH=\"${libdir}")
if(NS3_LIBRARY_PATH)
  set(CMAKE_SMPI_COMMAND "${CMAKE_SMPI_COMMAND}:${NS3_LIBRARY_PATH}")
endif()
set(CMAKE_SMPI_COMMAND "${CMAKE_SMPI_COMMAND}\${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}\"")
set(SMPIMAIN ${libdir}/simgrid/smpimain)
set(SMPIREPLAYMAIN ${libdir}/simgrid/smpireplaymain)

configure_file(${CMAKE_HOME_DIRECTORY}/include/smpi/mpif.h.in ${CMAKE_BINARY_DIR}/include/smpi/mpif.h @ONLY)
#configure mpif.f90 to build mpi.mod
if(SMPI_FORTRAN)
  set(MODULE_MPIF_IN "module mpi")
  set(MODULE_MPIF_OUT "end module mpi")
  configure_file(${CMAKE_HOME_DIRECTORY}/include/smpi/mpif.h.in ${CMAKE_BINARY_DIR}/src/smpi/mpif.f90.generated @ONLY)
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/src/smpi/mpif.f90.generated ${CMAKE_BINARY_DIR}/src/smpi/mpif.f90)
  set(CMAKE_Fortran_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/include/smpi)
  add_library(mpi SHARED ${CMAKE_BINARY_DIR}/src/smpi/mpif.f90)
endif()

foreach(script cc cxx ff f90 run)
  configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpi${script}.in ${CMAKE_BINARY_DIR}/bin/smpi${script} @ONLY)
endforeach()

### SMPI scripts used when compiling simgrid 
set(exec_prefix "${CMAKE_BINARY_DIR}/smpi_script/")
set(includedir "${CMAKE_HOME_DIRECTORY}/include")
set(libdir "${CMAKE_BINARY_DIR}/lib")
set(includeflag "-I${includedir} -I${includedir}/smpi")
set(includeflag "${includeflag} -I${CMAKE_BINARY_DIR}/include -I${CMAKE_BINARY_DIR}/include/smpi")
set(CMAKE_SMPI_COMMAND "export LD_LIBRARY_PATH=\"${libdir}")
if(NS3_LIBRARY_PATH)
  set(CMAKE_SMPI_COMMAND "${CMAKE_SMPI_COMMAND}:${NS3_LIBRARY_PATH}")
endif()
set(CMAKE_SMPI_COMMAND "${CMAKE_SMPI_COMMAND}\${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}\"")
set(SMPIMAIN ${libdir}/simgrid/smpimain)
set(SMPIREPLAYMAIN ${libdir}/simgrid/smpireplaymain)

foreach(script cc cxx ff f90 run)
  configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpi${script}.in ${CMAKE_BINARY_DIR}/smpi_script/bin/smpi${script} @ONLY)
endforeach()

if(NOT WIN32)
  foreach(script cc cxx ff f90 run)
    execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpi${script})
    execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/smpi_script/bin/smpi${script})
  endforeach()
endif()

set(generated_headers_to_install
  ${CMAKE_CURRENT_BINARY_DIR}/include/smpi/mpif.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/simgrid/config.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/simgrid/version.h
  )

set(generated_headers  ${CMAKE_CURRENT_BINARY_DIR}/src/internal_config.h )

set(generated_files_to_clean
  ${generated_headers}
  ${generated_headers_to_install}
  ${CMAKE_BINARY_DIR}/bin/smpicc
  ${CMAKE_BINARY_DIR}/bin/smpicxx
  ${CMAKE_BINARY_DIR}/bin/smpiff
  ${CMAKE_BINARY_DIR}/bin/smpif90
  ${CMAKE_BINARY_DIR}/bin/smpirun
  ${CMAKE_BINARY_DIR}/bin/colorize
  ${CMAKE_BINARY_DIR}/bin/simgrid_update_xml
  ${CMAKE_BINARY_DIR}/examples/smpi/tracing/smpi_traced.trace
  )

if(NOT "${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_HOME_DIRECTORY}")
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/actions0.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions0.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/actions1.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions1.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/actions_allreduce.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_allreduce.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/actions_barrier.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_barrier.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/actions_bcast.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_bcast.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/actions_with_isend.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_with_isend.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/actions_alltoall.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_alltoall.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/actions_alltoallv.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_alltoallv.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/actions_waitall.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_waitall.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/actions_reducescatter.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_reducescatter.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/actions_gather.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_gather.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/actions_allgatherv.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_allgatherv.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/hostfile ${CMAKE_BINARY_DIR}/teshsuite/smpi/hostfile COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/hostfile_cluster ${CMAKE_BINARY_DIR}/teshsuite/smpi/hostfile_cluster COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/hostfile_coll ${CMAKE_BINARY_DIR}/teshsuite/smpi/hostfile_coll COPYONLY)

  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/description_file ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/description_file COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/README ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/README COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/smpi_replay.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/smpi_replay.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace0.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace0.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace1.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace1.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace2.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace2.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace3.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace3.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace4.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace4.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace5.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace5.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace6.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace6.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace7.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace7.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace8.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace8.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace9.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace9.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace10.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace10.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace11.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace11.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace12.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace12.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace13.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace13.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace14.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace14.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace15.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace15.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace16.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace16.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace17.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace17.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace18.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace18.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace19.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace19.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace20.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace20.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace21.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace21.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace22.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace22.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace23.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace23.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace24.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace24.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace25.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace25.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace26.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace26.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace27.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace27.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace28.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace28.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace29.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace29.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace30.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace30.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace31.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace31.txt COPYONLY)

configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple_manual_deploy/compute_only.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/compute_only.txt COPYONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple_manual_deploy/compute_only/actions0.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/compute_only/actions0.txt COPYONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple_manual_deploy/compute_only/actions1.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/compute_only/actions1.txt COPYONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple_manual_deploy/empty.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/empty.txt COPYONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple_manual_deploy/empty/actions0.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/empty/actions0.txt COPYONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple_manual_deploy/empty/actions1.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/empty/actions1.txt COPYONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple_manual_deploy/mixed.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/mixed.txt COPYONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple_manual_deploy/mixed/actions0.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/mixed/actions0.txt COPYONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple_manual_deploy/mixed/actions1.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/mixed/actions1.txt COPYONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple_manual_deploy/workload_compute ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_compute COPYONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple_manual_deploy/workload_compute_consecutive ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_compute_consecutive COPYONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple_manual_deploy/workload_compute_consecutive2 ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_compute_consecutive2 COPYONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple_manual_deploy/workload_compute_simple ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_compute_simple COPYONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple_manual_deploy/workload_mixed2_same_time ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_mixed2_same_time COPYONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay_multiple_manual_deploy/workload_mixed2_same_time_and_resources ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_mixed2_same_time_and_resources COPYONLY)

  set(generated_files_to_clean
    ${generated_files_to_clean}
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions0.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions1.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_allreduce.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_barrier.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_bcast.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_with_isend.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_alltoall.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_alltoallv.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_waitall.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_gather.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_allgatherv.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_reducescatter.txt
    ${CMAKE_BINARY_DIR}/teshsuite/smpi/hostfile
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/description_file
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/README
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/smpi_replay.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace0.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace1.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace2.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace3.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace4.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace5.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace6.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace7.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace8.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace9.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace10.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace11.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace12.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace13.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace14.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace15.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace16.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace17.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace18.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace19.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace20.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace21.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace22.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace23.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace24.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace25.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace26.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace27.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace28.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace29.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace30.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple/ti_traces_32_1/ti_trace31.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/compute_only.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/compute_only/actions0.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/compute_only/actions1.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/empty.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/empty/actions0.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/empty/actions1.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/mixed.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/mixed/actions0.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/mixed/actions1.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/replay_multiple_manual.tesh
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_compute
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_compute_consecutive
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_compute_consecutive2
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_compute_simple
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_empty1
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_empty2
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_empty2_same_resources
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_empty2_same_time
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_empty2_same_time_and_resources
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_mixed1
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_mixed2
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_mixed2_same_resources
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_mixed2_same_time
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_mixed2_same_time_and_resources
    ${CMAKE_BINARY_DIR}/examples/smpi/replay_multiple_manual_deploy/workload_nojob
    )
endif()

SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
  "${generated_files_to_clean}")

add_custom_target(tests COMMENT "Recompiling the tests")

### Build some Maintainer files
include(${CMAKE_HOME_DIRECTORY}/tools/cmake/MaintainerMode.cmake)

### Make Libs
if(NOT WIN32)
  include(${CMAKE_HOME_DIRECTORY}/tools/cmake/MakeLib.cmake)
else()
  include(${CMAKE_HOME_DIRECTORY}/tools/cmake/MakeLibWin.cmake)
endif()

if(enable_java)
  include(${CMAKE_HOME_DIRECTORY}/tools/cmake/Java.cmake)
endif()

# Python binding (with pybind11)
################
if((NOT DEFINED enable_python) OR enable_python)
  if(EXISTS ${CMAKE_HOME_DIRECTORY}/pybind11) # Try to use a local copy of pybind11, if any
    message(STATUS "Use the internal copy of pybind11.")
    add_subdirectory(${CMAKE_HOME_DIRECTORY}/pybind11)
    set(pybind11_FOUND ON)

    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_HOME_DIRECTORY}/pybind11/tools/)
    set(Python_ADDITIONAL_VERSIONS 3.9 3.8 3.7 3.6 3.5 3.4)
    find_package(PythonLibsNew ${PYBIND11_PYTHON_VERSION} REQUIRED)

  else()
    find_package(pybind11 CONFIG)
    message(STATUS "Pybind11 version: ${pybind11_VERSION}")
    if (pybind11_VERSION VERSION_LESS 2.4)
      message(STATUS "SimGrid needs at least v2.4 of pybind11. Disabling the Python bindings.")
      set(pybind11_FOUND OFF)
    endif()
  endif()

  if(NOT PYTHONLIBS_FOUND AND NOT Python3_Development_FOUND)
    message(STATUS "Python libs not found. Turn pybind11 off.")

    set(pybind11_FOUND OFF)
  endif()
endif()

option(enable_python "Whether the Python bindings are activated." ${pybind11_FOUND}) # ON by default if dependencies are met

if("${CMAKE_SYSTEM}" MATCHES "FreeBSD" AND enable_model-checking AND enable_python)
  message(WARNING "FreeBSD + Model-Checking + Python = too much for now. Disabling the Python bindings.")
  set(enable_python FALSE)
endif()

if(enable_python)
  if(pybind11_FOUND)
    message(STATUS "Found pybind11.")
    if(NOT enable_lto)
      set(pybind11_options NO_EXTRAS)
    endif()
    pybind11_add_module(python-bindings src/bindings/python/simgrid_python.cpp ${pybind11_options})
    target_compile_features(python-bindings PRIVATE cxx_std_14)
    target_link_libraries(python-bindings PUBLIC simgrid)
    set_target_properties(python-bindings PROPERTIES
                          LIBRARY_OUTPUT_NAME simgrid
                          CXX_VISIBILITY_PRESET "default"
                          INTERPROCEDURAL_OPTIMIZATION FALSE)
    # LTO is disabled here from the python bindings because this makes a
    # cmake warning about CMP0069 even when this policy is set. This
    # problem may be in cmake, in pybind11 or even in our code, not sure.
    # It may get eventually solved in cmake or pybind11. Or not.
    # The sure thing is that our python bindings are in one file only,
    # so there is no need for LTO here. Problem solved :)

    add_dependencies(tests python-bindings)
    set_property(TARGET python-bindings
                 APPEND PROPERTY INCLUDE_DIRECTORIES "${INTERNAL_INCLUDES}")
  else()
    message(FATAL_ERROR "Please install pybind11-dev to build the Python bindings (or disable that option).")
  endif()
endif()
mark_as_advanced(PYBIND11_PYTHON_VERSION)
mark_as_advanced(pybind11_DIR)

### Make tests
if(enable_memcheck_xml)
  set(enable_memcheck true)
endif()

INCLUDE(CTest)
ENABLE_TESTING()
include(${CMAKE_HOME_DIRECTORY}/tools/cmake/Tests.cmake)
include(${CMAKE_HOME_DIRECTORY}/tools/cmake/CTestConfig.cmake)

### Define subdirectories
foreach(cmakefile ${CMAKEFILES_TXT})
  string(REPLACE "/CMakeLists.txt" "" repository ${cmakefile})
  add_subdirectory("${CMAKE_HOME_DIRECTORY}/${repository}")
endforeach()

### Setup the distrib
include(${CMAKE_HOME_DIRECTORY}/tools/cmake/Distrib.cmake)

### Build the docs if asked to
include(${CMAKE_HOME_DIRECTORY}/tools/cmake/Documentation.cmake)

### Print the result of configuration
message("")
message("##########################################")
message("#### Content of src/internal_config.h ####")
message("##########################################")
file(STRINGS ${CMAKE_CURRENT_BINARY_DIR}/src/internal_config.h config_output)
LIST(REMOVE_AT config_output 0 1 2 3 4 5 6 7 8 9 10) # Pass the file header
foreach(line ${config_output})
  message("   ${line}")
endforeach()
message("##########################################")
message("####   Content of simgrid/config.h    ####")
message("##########################################")
file(STRINGS ${CMAKE_CURRENT_BINARY_DIR}/include/simgrid/config.h config_output)
LIST(REMOVE_AT config_output 0 1 2 3 4 5 6 7 8 9 -1) # Pass the file header
foreach(line ${config_output})
  message("   ${line}")
endforeach()
message("##########################################")
message("####   End of configuration headers   ####")
message("##########################################")

message("\nConfiguration of package `simgrid':")
message("        Home directory ..............: ${CMAKE_HOME_DIRECTORY}")
message("        Build Name ..................: ${BUILDNAME}")
message("        Cmake Generator .............: ${CMAKE_GENERATOR}")
message("        Site ........................: ${SITE}")
message("        Install prefix ..............: ${CMAKE_INSTALL_PREFIX}")
message("        Release .....................: simgrid-${release_version}")
message("")
message("        Compiler: C .................: ${CMAKE_C_COMPILER} (id: ${CMAKE_C_COMPILER_ID})")
message("                version .............: ${CMAKE_C_COMPILER_VERSION}")
message("                is gnu ..............: ${CMAKE_COMPILER_IS_GNUCC}")
message("        Compiler: C++ ...............: ${CMAKE_CXX_COMPILER} (id: ${CMAKE_CXX_COMPILER_ID})")
message("                version .............: ${CMAKE_CXX_COMPILER_VERSION}")
if(${Java_FOUND})
  message("        Compiler: Javac .............: ${Java_JAVAC_EXECUTABLE}")
  message("                version .............: ${Java_VERSION_STRING}")
  message("                runtime .............: ${Java_JAVA_EXECUTABLE}")
endif()
if(CMAKE_Fortran_COMPILER)
  message("        Compiler: Fortran ...........: ${SMPI_Fortran_COMPILER} (id: ${CMAKE_Fortran_COMPILER_ID})")
  message("                version .............: ${CMAKE_Fortran_COMPILER_VERSION}")
endif()
message("        Linker: .....................: ${CMAKE_LINKER}")
message("                version .............: ${LINKER_VERSION}")
message("        Make program: ...............: ${CMAKE_MAKE_PROGRAM}")
message("")
message("        CFlags ......................: ${CMAKE_C_FLAGS}")
message("        CXXFlags ....................: ${CMAKE_CXX_FLAGS}")
message("        LDFlags .....................: ${CMAKE_C_LINK_FLAGS}")
message("        with LTO ....................: ${enable_lto}")
message("")

if (SIMGRID_HAVE_MSG)
  message("        Compile MSG .................: ON")
else()
  message("        Compile MSG .................: OFF")
endif()

if (SIMGRID_HAVE_NS3)
  message("        Compile ns-3 ................: ON (path: ${NS3_PATH})")
else()
  message("        Compile ns-3 ................: OFF  (hint: ${NS3_HINT})")
endif()

if (${Java_FOUND})
  message("        Compile Java ................: ON")
  message("          Native lib in jar .........: ${enable_lib_in_jar}")
else()
  message("        Compile Java ................: OFF")
endif()
if(pybind11_FOUND)
  message("        Compile Python bindings .....: ${enable_python}")
  message("          module ....................: ${PYTHON_MODULE_PREFIX}simgrid${PYTHON_MODULE_EXTENSION}")
else()
  message("        Compile Python bindings .....: OFF (disabled, or pybind11 not found)")
endif()
message("        Compile Lua .................: ${SIMGRID_HAVE_LUA}")
message("        Compile Smpi ................: ${HAVE_SMPI}")
message("          Smpi fortran ..............: ${SMPI_FORTRAN}")
message("          MPICH3 testsuite ..........: ${enable_smpi_MPICH3_testsuite}")
message("          Privatization .............: ${HAVE_PRIVATIZATION}")
message("          PAPI support...............: ${HAVE_PAPI}")
message("        Compile Boost.Context support: ${HAVE_BOOST_CONTEXTS}")
message("")
message("        Maintainer mode .............: ${enable_maintainer_mode}")
message("        Documentation................: ${enable_documentation}")
message("        Model checking ..............: ${SIMGRID_HAVE_MC}")
message("        Jedule  mode ................: ${SIMGRID_HAVE_JEDULE}")
message("        Graphviz mode ...............: ${HAVE_GRAPHVIZ}")
message("        Mallocators .................: ${enable_mallocators}")
message("")
message("        Simgrid dependencies ........: ${SIMGRID_DEP}")
message("")

execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/Testing/Notes/)
file(WRITE ${PROJECT_BINARY_DIR}/Testing/Notes/Build  "GIT version : ${GIT_VERSION}\n")
file(APPEND ${PROJECT_BINARY_DIR}/Testing/Notes/Build "Release     : simgrid-${release_version}\n")

INCLUDE(Dart)
