cmake_minimum_required(VERSION 2.8.8)
message(STATUS "Cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_HOME_DIRECTORY}/tools/cmake/Modules)

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
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.7")
      message(FATAL_ERROR
              "SimGrid needs at least g++ version 4.7 to compile but you have ${CMAKE_CXX_COMPILER_VERSION}."
  	      "You need a sufficient support of c++11 to compile SimGrid.")
    endif()
  endif()

  ## We need a decent support of the c++11 standard
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("-std=gnu++11" COMPILER_SUPPORTS_CXX11)
  if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
  else() 
    message(FATAL_ERROR 
            "The compiler ${CMAKE_CXX_COMPILER} (v${CMAKE_CXX_COMPILER_VERSION}) has no C++11 support. "
            "Please use a decent C++ compiler.")
  endif()

  ### And we need C11 standard, too
  include(CheckCCompilerFlag)
  CHECK_C_COMPILER_FLAG("-std=gnu11" COMPILER_SUPPORTS_C11)
  if(COMPILER_SUPPORTS_C11)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
  else()
    message(FATAL_ERROR 
            "The compiler ${CMAKE_C_COMPILER} (v${CMAKE_C_COMPILER_VERSION}) has no C11 support. "
            "Please use a decent C compiler "
            "(note that c++11 support of ${CMAKE_CXX_COMPILER} seems ok).")
  endif()
  if(APPLE AND (CMAKE_C_COMPILER_VERSION VERSION_LESS "4.6"))
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
set(SIMGRID_VERSION_MINOR "13")
set(SIMGRID_VERSION_PATCH "0")
set(SIMGRID_VERSION_EXTRA "-devel") # Extra words to add to version string (e.g. -rc1)

set(SIMGRID_VERSION_DATE  "2016") # Year for copyright information

if(${SIMGRID_VERSION_PATCH} EQUAL "0")
  set(release_version "${SIMGRID_VERSION_MAJOR}.${SIMGRID_VERSION_MINOR}")
else()
  set(release_version "${SIMGRID_VERSION_MAJOR}.${SIMGRID_VERSION_MINOR}.${SIMGRID_VERSION_PATCH}")
endif()

set(SIMGRID_VERSION_STRING "SimGrid version ${release_version}${SIMGRID_VERSION_EXTRA}")

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

execute_process(COMMAND         ${CMAKE_LINKER} --version
                OUTPUT_VARIABLE LINKER_VERSION)
string(REGEX MATCH "[0-9].[0-9]*" LINKER_VERSION "${LINKER_VERSION}")

### Find programs and paths
FIND_PROGRAM(GCOV_PATH gcov)
include(FindPerl)
if(NOT PERL_FOUND)
  message(FATAL_ERROR "Please install Perl to compile SimGrid.")
endif()

if (APPLE)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

### Compute the include paths
set(INCLUDES
  ${CMAKE_HOME_DIRECTORY}
  ${CMAKE_HOME_DIRECTORY}/include
  ${CMAKE_HOME_DIRECTORY}/src/include
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/include
  )

if(NOT CMAKE_CROSSCOMPILING AND EXISTS /usr/include/)
  set(INCLUDES ${INCLUDES} /usr/include/)
endif()

### Check 32bits or 64bits
IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
  SET(ARCH_32_BITS 1)
ELSE()
  SET(ARCH_32_BITS 0)
ENDIF()

if(WIN32)

  set(CMAKE_INCLUDE_WIN "${CMAKE_C_COMPILER}")
  set(CMAKE_LIB_WIN "${CMAKE_C_COMPILER}")
  string(REGEX REPLACE "/bin/gcc.*" "/include"  CMAKE_INCLUDE_WIN "${CMAKE_INCLUDE_WIN}")
  string(REGEX REPLACE "/bin/gcc.*" "/lib"  CMAKE_LIB_WIN "${CMAKE_LIB_WIN}")
  set(INCLUDES ${INCLUDES} ${CMAKE_INCLUDE_WIN})
  unset(CMAKE_INCLUDE_WIN)

endif()

include_directories(${INCLUDES})

### Setup Options
include(${CMAKE_HOME_DIRECTORY}/tools/cmake/Option.cmake)

### Determine the assembly flavor that we need today
set(HAVE_RAW_CONTEXTS 0)
include(CMakeDetermineSystem)
IF(CMAKE_SYSTEM_PROCESSOR MATCHES ".86|AMD64|amd64")
  IF(${ARCH_32_BITS})
    message(STATUS "System processor: i686 (${CMAKE_SYSTEM_PROCESSOR}, 32 bits)")
    set(PROCESSOR_i686 1)
    set(PROCESSOR_x86_64 0)
  ELSE()
    message(STATUS "System processor: x86_64 (${CMAKE_SYSTEM_PROCESSOR}, 64 bits)")
    set(PROCESSOR_i686 0)
    set(PROCESSOR_x86_64 1)
  ENDIF()
  if (WIN32)
    message(STATUS "Disable fast raw contexts on Windows.")
  else()
    set(HAVE_RAW_CONTEXTS 1)
  endif()
ELSE()
  set(PROCESSOR_i686 0)
  set(PROCESSOR_x86_64 0)
ENDIF()

include(CheckFunctionExists)
include(CheckTypeSize)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckSymbolExists)

set(HAVE_GRAPHVIZ 0)
include(FindGraphviz)
include(FindLibSigc++)

set(HAVE_LUA 0)
if(enable_lua)
  include(FindLuaSimgrid)
endif()

set(HAVE_NS3 0)
if(enable_ns3)
  include(FindNS3)
  if (NOT HAVE_NS3)
    message(FATAL_ERROR "Cannot find NS3. Please install it (apt-get install ns3 libns3-dev) or disable that cmake option")
  endif()
endif()

if(WIN32)
  set(Boost_USE_STATIC_LIBS 1)
endif()

find_package(Boost 1.48)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
else()
  if(APPLE)
    message(FATAL_ERROR "Failed to find Boost libraries (Try to install them with 'sudo fink install boost1.53.nopython')")
  else()
    message(FATAL_ERROR "Failed to find Boost libraries."
                        "Did you install libboost-dev and libboost-context-dev?"
                        "(libboost-context-dev is optional)"
                        "SimGrid requires Boost >= 1.48.0")
  endif()
endif()

# Try again to see if we have libboost-context
find_package(Boost 1.42 COMPONENTS context)
set(Boost_FOUND 1) # We don't care of whether this component is missing

if(Boost_FOUND AND Boost_CONTEXT_FOUND)
  # We should use feature detection for this instead:
  if (Boost_VERSION LESS 105600)
    message("Found Boost.Context API v1")
    set(HAVE_BOOST_CONTEXTS 1)
  else()
    message("Found Boost.Context API v2")
    set(HAVE_BOOST_CONTEXTS 2)
  endif()
else()
  message ("   boost        : found.")
  message ("   boost-context: missing. Install libboost-context-dev for this optional feature.")
  set(HAVE_BOOST_CONTEXTS 0)
endif()

# Checks for header libraries functions.
CHECK_LIBRARY_EXISTS(execinfo backtrace              "" HAVE_BACKTRACE_IN_LIBEXECINFO)
CHECK_LIBRARY_EXISTS(rt      clock_gettime           "" HAVE_POSIX_GETTIME)

CHECK_LIBRARY_EXISTS(pthread pthread_create          "" HAVE_PTHREAD)
CHECK_LIBRARY_EXISTS(pthread sem_init                "" HAVE_SEM_INIT_LIB)
CHECK_LIBRARY_EXISTS(pthread sem_open                "" HAVE_SEM_OPEN_LIB)
set(HAVE_PTHREAD_SETAFFINITY 0)
CHECK_LIBRARY_EXISTS(pthread pthread_setaffinity_np  "" HAVE_PTHREAD_SETAFFINITY)

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
CHECK_INCLUDE_FILE("execinfo.h" HAVE_EXECINFO_H)
CHECK_INCLUDE_FILE("signal.h" HAVE_SIGNAL_H)
CHECK_INCLUDE_FILE("sys/param.h" HAVE_SYS_PARAM_H)
CHECK_INCLUDE_FILE("sys/sysctl.h" HAVE_SYS_SYSCTL_H)
CHECK_INCLUDE_FILE("ucontext.h" HAVE_UCONTEXT_H)
CHECK_INCLUDE_FILE("linux/futex.h" HAVE_FUTEX_H)

CHECK_FUNCTION_EXISTS(gettimeofday HAVE_GETTIMEOFDAY)
CHECK_FUNCTION_EXISTS(nanosleep HAVE_NANOSLEEP)
CHECK_FUNCTION_EXISTS(getdtablesize HAVE_GETDTABLESIZE)
CHECK_FUNCTION_EXISTS(sysconf HAVE_SYSCONF)
CHECK_FUNCTION_EXISTS(popen HAVE_POPEN)
CHECK_FUNCTION_EXISTS(process_vm_readv HAVE_PROCESS_VM_READV)
CHECK_FUNCTION_EXISTS(mmap HAVE_MMAP)

CHECK_SYMBOL_EXISTS(vasprintf stdio.h HAVE_VASPRINTF)
if(MINGW)
  # The detection of vasprintf fails on MinGW, assumingly because it's
  # defined as an inline function in stdio.h instead of a regular
  # function. So force the result to be 1 despite of the test.
  set(HAVE_VASPRINTF 1)
endif()

#Check if __thread is defined
execute_process(
  COMMAND "${CMAKE_C_COMPILER} ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_thread_storage.c -o testprog"
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  RESULT_VARIABLE HAVE_thread_storage_run)
file(REMOVE testprog)
if(HAVE_thread_storage_run)
  set(HAVE_THREAD_LOCAL_STORAGE 1)
else()
  set(HAVE_THREAD_LOCAL_STORAGE 0)
endif()

# Our usage of mmap is Linux-specific (flag MAP_ANONYMOUS), but kFreeBSD uses a GNU libc
IF(HAVE_MMAP AND
   NOT "${CMAKE_SYSTEM}" MATCHES "Linux" AND 
   NOT "${CMAKE_SYSTEM}" MATCHES "kFreeBSD" AND 
   NOT "${CMAKE_SYSTEM}" MATCHES "GNU" AND 
   NOT  "${CMAKE_SYSTEM}" MATCHES "Darwin")
  SET(HAVE_MMAP 0)
  message(STATUS "Warning: MMAP is thought as non functional on this architecture (${CMAKE_SYSTEM})")
ENDIF()

if(HAVE_MMAP AND HAVE_THREAD_LOCAL_STORAGE)
  SET(HAVE_MMALLOC 1)
else()
  SET(HAVE_MMALLOC 0)
endif()

if(enable_jedule)
  set(HAVE_JEDULE 1)
else()
  set(HAVE_JEDULE 0)
endif()

if(enable_mallocators)
  SET(HAVE_MALLOCATOR 1)
else()
  SET(HAVE_MALLOCATOR 0)
endif()

if(enable_model-checking AND HAVE_MMALLOC)
  SET(HAVE_MC 1)
  include(FindLibunwind)
  include(FindLibdw)
else()
  if(enable_model-checking)
    message(STATUS "Warning: support for model-checking has been disabled because you are missing either mmap or __thread.")
  endif()
  SET(HAVE_MC 0)
  SET(HAVE_MMALLOC 0)
endif()

if(enable_smpi)
  include(FindGFortran)
  SET(HAVE_SMPI 1)
  if("${CMAKE_SYSTEM}" MATCHES "Linux")
    SET(USE_LIBUTIL 0)
    SET(HAVE_PRIVATIZATION 1)
  elseif("${CMAKE_SYSTEM}" MATCHES "^FreeBSD")
    SET(USE_LIBUTIL 0)
    SET(HAVE_PRIVATIZATION 0)
  else()
    message (STATUS "Warning:  no support for SMPI automatic privatization on this platform")
    SET(HAVE_PRIVATIZATION 0)
  endif()
else()
  SET(HAVE_SMPI 0)
endif()

#--------------------------------------------------------------------------------------------------
### Check for GNU dynamic linker
CHECK_INCLUDE_FILE("dlfcn.h" HAVE_DLFCN_H)
if (HAVE_DLFCN_H)
    execute_process(COMMAND ${CMAKE_C_COMPILER} ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_gnu_dynlinker.c -ldl -o test_gnu_ld
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      OUTPUT_VARIABLE HAVE_GNU_LD_compil
    )
    if(HAVE_GNU_LD_compil)
      set(HAVE_GNU_LD 0)
      message(STATUS "Warning: test program toward GNU ld failed to compile:")
      message(STATUS "${HAVE_GNU_LD_comp_output}")
    else()

      execute_process(COMMAND ./test_gnu_ld
          WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
          RESULT_VARIABLE HAVE_GNU_LD_run
          OUTPUT_VARIABLE var_exec
      )

      if(NOT HAVE_GNU_LD_run)
        set(HAVE_GNU_LD 1)
        message(STATUS "We are using GNU dynamic linker")
      else()
        set(HAVE_GNU_LD 0)
        message(STATUS "Warning: error while checking for GNU ld:")
        message(STATUS "Test output: '${var_exec}'")
        message(STATUS "Exit status: ${HAVE_GNU_LD_run}")
      endif()
      file(REMOVE test_gnu_ld)
    endif()
endif()


#--------------------------------------------------------------------------------------------------
### Initialize of CONTEXT THREADS

set(HAVE_THREAD_CONTEXTS 0)

if(HAVE_PTHREAD)
  ### Test that we have a way to create semaphores

  if(HAVE_SEM_OPEN_LIB)
    execute_process(COMMAND ${CMAKE_C_COMPILER} ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_sem_open.c -lpthread -o sem_open
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      OUTPUT_VARIABLE HAVE_SEM_OPEN_compil
    )

    # Test sem_open by compiling:
    if(HAVE_SEM_OPEN_compil)
      set(HAVE_SEM_OPEN 0)
      message(STATUS "Warning: sem_open not compilable")
      message(STATUS "HAVE_SEM_OPEN_comp_output: ${HAVE_SEM_OPEN_comp_output}")
    else()
      set(HAVE_SEM_OPEN 1)
      message(STATUS "sem_open is compilable")
    endif()

    # If we're not crosscompiling, we check by executing the program:
    if (HAVE_SEM_OPEN AND NOT CMAKE_CROSSCOMPILING)
      execute_process(COMMAND ./sem_open
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        RESULT_VARIABLE HAVE_SEM_OPEN_run
        OUTPUT_VARIABLE var_compil
      )
      if (NOT HAVE_SEM_OPEN_run)
        set(HAVE_SEM_OPEN 1)
        message(STATUS "sem_open is executable")
      else()
        set(HAVE_SEM_OPEN 0)
        if(EXISTS "${CMAKE_BINARY_DIR}/sem_open")
          message(STATUS "Bin ${CMAKE_BINARY_DIR}/sem_open exists!")
        else()
          message(STATUS "Bin ${CMAKE_BINARY_DIR}/sem_open not exists!")
        endif()
        message(STATUS "Warning: sem_open not executable")
        message(STATUS "Compilation output: '${var_compil}'")
        message(STATUS "Exit result of sem_open: ${HAVE_SEM_OPEN_run}")
      endif()
    endif()
    file(REMOVE sem_open)

  else()
    set(HAVE_SEM_OPEN 0)
  endif()

  if(HAVE_SEM_INIT_LIB)
    execute_process(COMMAND ${CMAKE_C_COMPILER} ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_sem_init.c -lpthread -o sem_init
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      RESULT_VARIABLE HAVE_SEM_INIT_run 
      OUTPUT_VARIABLE HAVE_SEM_INIT_compil)

    # Test sem_init by compiling:
    if(HAVE_SEM_INIT_compil)
      set(HAVE_SEM_INIT 0)
      message(STATUS "Warning: sem_init not compilable")
      message(STATUS "HAVE_SEM_INIT_comp_output: ${HAVE_SEM_OPEN_comp_output}")
    else()
      set(HAVE_SEM_INIT 1)
      message(STATUS "sem_init is compilable")
    endif()

    # If we're not crosscompiling, we check by executing the program:
    if (HAVE_SEM_INIT AND NOT CMAKE_CROSSCOMPILING)
      execute_process(COMMAND ./sem_init
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        RESULT_VARIABLE HAVE_SEM_INIT_run
        OUTPUT_VARIABLE var_compil
      )
      if (NOT HAVE_SEM_INIT_run)
        set(HAVE_SEM_INIT 1)
        message(STATUS "sem_init is executable")
      else()
        set(HAVE_SEM_INIT 0)
        if(EXISTS "${CMAKE_BINARY_DIR}/sem_init")
          message(STATUS "Bin ${CMAKE_BINARY_DIR}/sem_init exists!")
        else()
          message(STATUS "Bin ${CMAKE_BINARY_DIR}/sem_init not exists!")
        endif()
        message(STATUS "Warning: sem_init not executable")
        message(STATUS "Compilation output: '${var_compil}'")
        message(STATUS "Exit result of sem_init: ${HAVE_SEM_INIT_run}")
      endif()
    endif()
    file(REMOVE sem_init)
  endif()

  if(NOT HAVE_SEM_OPEN AND NOT HAVE_SEM_INIT)
    message(FATAL_ERROR "Semaphores are not usable (neither sem_open nor sem_init is both compilable and executable), but they are mandatory to threads (you may need to mount /dev).")
  endif()

  set(HAVE_THREAD_CONTEXTS 1)
  message("-- Support for thread context factory ok.")
endif()


set(HAVE_UCONTEXT_CONTEXTS 0)
if(NOT HAVE_UCONTEXT_H)
  message("-- No ucontext factory: <ucontext.h> not found.")
elseif(APPLE)
  message("-- No ucontext factory: Apple don't want us to use them.")
  set(HAVE_UCONTEXT_H 0)
else()
  try_compile(compile_makecontext ${CMAKE_BINARY_DIR} ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_makecontext.c
    OUTPUT_VARIABLE compile_makecontext_output)
  
  #If can have both context
  if(compile_makecontext)
    set(HAVE_UCONTEXT_CONTEXTS 1)
    message("-- Support for ucontext factory ok.")
  else()
    message("-- Error: <ucontext.h> exists, but makecontext is not compilable. Compilation output:\n ${compile_makecontext_output}")
    message("-- No ucontext factory: makecontext() is not compilable.")
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

### check for addr2line
find_path(ADDR2LINE NAMES addr2line	PATHS NO_DEFAULT_PATHS)
if(ADDR2LINE)
  set(ADDR2LINE "${ADDR2LINE}/addr2line")
endif()

###############
## GIT version check
##
if(EXISTS ${CMAKE_HOME_DIRECTORY}/.git/)
  execute_process(
     COMMAND git remote
     COMMAND head -n 1
     WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/.git/
     OUTPUT_VARIABLE remote
     OUTPUT_STRIP_TRAILING_WHITESPACE)
  #message(STATUS "Git remote: ${remote}")
  execute_process(COMMAND git config --get remote.${remote}.url
     WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/.git/
     OUTPUT_VARIABLE url
     OUTPUT_STRIP_TRAILING_WHITESPACE)
  #message(STATUS "Git url: ${url}")
  if(url)
    execute_process(COMMAND git --git-dir=${CMAKE_HOME_DIRECTORY}/.git log --pretty=oneline --abbrev-commit -1
       WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/.git/
       OUTPUT_VARIABLE GIT_VERSION
       OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Git version: ${GIT_VERSION}")
    
    execute_process(COMMAND git --git-dir=${CMAKE_HOME_DIRECTORY}/.git log -n 1 --pretty=format:%ai .
       WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/.git/
       OUTPUT_VARIABLE GIT_DATE 
       OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Git date: ${GIT_DATE}")
    string(REGEX REPLACE " .*" "" GIT_VERSION "${GIT_VERSION}")
    
    execute_process(COMMAND git --git-dir=${CMAKE_HOME_DIRECTORY}/.git log --pretty=format:%H -1
       WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/.git/
       OUTPUT_VARIABLE SIMGRID_GITHASH
       OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
elseif(EXISTS ${CMAKE_HOME_DIRECTORY}/.gitversion)
  FILE(STRINGS ${CMAKE_HOME_DIRECTORY}/.gitversion GIT_VERSION)
endif()


### Generate the required headers and scripts
#############################################

# gcc makes no difference between "#define HAVE_FOOBAR" and "#define HAVE_FOOBAR 0" by default, 
# but this too error prone. If you forget to include the config.h, you get weird segfaults.
# If you include it everywhere, everything gets recompiled everytime.
# So we include only where needed, and compile with -Wundef to notice the missing includes.
# But cmake sometimes defines to the empty definition (#define HAVE_VALGRIND_H).
# So we have to make sure that everything got a decent value before generating the files.
foreach(var HAVE_EXECINFO_H HAVE_FUTEX_H HAVE_GETDTABLESIZE HAVE_GETTIMEOFDAY HAVE_MMAP
            HAVE_NANOSLEEP  HAVE_POPEN HAVE_POSIX_GETTIME HAVE_PROCESS_VM_READV HAVE_SIGNAL_H
	    HAVE_SYS_PARAM_H HAVE_SYS_SYSCTL_H HAVE_SYSCONF HAVE_UCONTEXT_H HAVE_UNISTD_H
	    HAVE_VALGRIND_H HAVE_VASPRINTF)
  if(${var})
    set(${var} 1)
  else()
    set(${var} 0)
  endif()
endforeach()

# Avoid triggering a (full) rebuild by touching the files if they did not really change
configure_file("${CMAKE_HOME_DIRECTORY}/tools/cmake/src/internal_config.h.in" "${CMAKE_BINARY_DIR}/src/internal_config.h.generated"    @ONLY IMMEDIATE)
configure_file("${CMAKE_HOME_DIRECTORY}/include/simgrid_config.h.in"          "${CMAKE_BINARY_DIR}/include/simgrid_config.h.generated" @ONLY IMMEDIATE)
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/src/internal_config.h.generated ${CMAKE_BINARY_DIR}/src/internal_config.h)
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/include/simgrid_config.h.generated ${CMAKE_BINARY_DIR}/include/simgrid_config.h)
file(REMOVE ${CMAKE_BINARY_DIR}/src/internal_config.h.generated)
file(REMOVE ${CMAKE_BINARY_DIR}/include/simgrid_config.h.generated)

# We need two versions of the SMPI scripts because they contain the path to the library
# so, it depends of whether SimGrid is installed, or run from the sources (during the build)

file(READ ${CMAKE_HOME_DIRECTORY}/src/smpi/smpitools.sh SMPITOOLS_SH) # Definitions shared amongst all SMPI scripts, inlined in each of them

### SMPI script used when simgrid is installed
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(includeflag "-I${CMAKE_INSTALL_PREFIX}/include -I${CMAKE_INSTALL_PREFIX}/include/smpi")
set(includedir "${CMAKE_INSTALL_PREFIX}/include")
set(libdir ${exec_prefix}/lib)
set(CMAKE_SMPI_COMMAND "export LD_LIBRARY_PATH=\"${CMAKE_INSTALL_PREFIX}/lib")
if(NS3_LIBRARY_PATH)
  set(CMAKE_SMPI_COMMAND "${CMAKE_SMPI_COMMAND}:${NS3_LIBRARY_PATH}")
endif()
set(CMAKE_SMPI_COMMAND "${CMAKE_SMPI_COMMAND}:\${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}\"")

configure_file(${CMAKE_HOME_DIRECTORY}/include/smpi/mpif.h.in ${CMAKE_BINARY_DIR}/include/smpi/mpif.h @ONLY)
foreach(script cc cxx ff f90 run)
  configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpi${script}.in ${CMAKE_BINARY_DIR}/bin/smpi${script} @ONLY)
endforeach()

### SMPI scripts used when compiling simgrid 
set(exec_prefix "${CMAKE_BINARY_DIR}/smpi_script/")
set(includeflag "-I${CMAKE_HOME_DIRECTORY}/include -I${CMAKE_HOME_DIRECTORY}/include/smpi")
set(includeflag "${includeflag} -I${CMAKE_BINARY_DIR}/include -I${CMAKE_BINARY_DIR}/include/smpi")
set(includedir "${CMAKE_HOME_DIRECTORY}/include")
set(libdir "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_SMPI_COMMAND "export LD_LIBRARY_PATH=\"${CMAKE_BINARY_DIR}/lib")
if(NS3_LIBRARY_PATH)
  set(CMAKE_SMPI_COMMAND "${CMAKE_SMPI_COMMAND}:${NS3_LIBRARY_PATH}")
endif()
set(CMAKE_SMPI_COMMAND "${CMAKE_SMPI_COMMAND}:\${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}\"")

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
  ${CMAKE_CURRENT_BINARY_DIR}/include/simgrid_config.h
  )

set(generated_headers
  ${CMAKE_CURRENT_BINARY_DIR}/src/internal_config.h
  )

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
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/actions_allReduce.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_allReduce.txt COPYONLY)
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

  set(generated_files_to_clean
    ${generated_files_to_clean}
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions0.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions1.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_allReduce.txt
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
    )
endif()

SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
  "${generated_files_to_clean}")

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

### Build the doc (Windows downloads the doc instead of regenerating)
#
if(NOT WIN32)
  include(${CMAKE_HOME_DIRECTORY}/tools/cmake/Documentation.cmake)
else()
  find_program(WGET_PROGRAM  NAMES wget)
  message(STATUS "wget: ${WGET_PROGRAM}")
  if(WGET_PROGRAM)
    ADD_CUSTOM_TARGET(documentation
      COMMENT "Downloading the SimGrid documentation..."
      COMMAND ${WGET_PROGRAM} -r -np -nH -nd http://simgrid.gforge.inria.fr/simgrid/${release_version}/doc/
      WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/html
    )
  endif()
endif()

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
message("####   Content of simgrid_config.h    ####")
message("##########################################")
file(STRINGS ${CMAKE_CURRENT_BINARY_DIR}/include/simgrid_config.h config_output)
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
if(release)
  message("        Release .....................: simgrid-${release_version}${SIMGRID_VERSION_EXTRA} (release build)")
else()
  message("        Release .....................: simgrid-${release_version}${SIMGRID_VERSION_EXTRA} (development build)")
endif()
message("")
message("        Compiler: C .................: ${CMAKE_C_COMPILER} (id: ${CMAKE_C_COMPILER_ID})")
message("                version .............: ${CMAKE_C_COMPILER_VERSION}")
message("                is gnu ..............: ${CMAKE_COMPILER_IS_GNUCC}")
message("        Compiler: C++ ...............: ${CMAKE_CXX_COMPILER} (id: ${CMAKE_CXX_COMPILER_ID})")
message("                version .............: ${CMAKE_CXX_COMPILER_VERSION}")
if(${Java_FOUND})
  message("        Compiler: Javac .............: ${Java_JAVAC_EXECUTABLE}")
  message("                version .............: ${Java_VERSION_STRING}")
if(${SWIG_FOUND})
  message("                SWIG ................: Version ${SWIG_VERSION} found.")
else()
  message("                SWIG ................: Missing.")
endif()
endif()
if(CMAKE_Fortran_COMPILER)
  message("        Compiler: Fortran ...........: ${CMAKE_Fortran_COMPILER} (id: ${CMAKE_Fortran_COMPILER_ID})")
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

if (HAVE_NS3)
  message("        Compile NS-3 ................: yes (path: ${NS3_PATH})")
else()
  message("        Compile NS-3 ................: NO  (hint: ${NS3_HINT})")
endif()

if (${Java_FOUND})
  message("        Compile Java ................: yes")
  message("          Native lib in jar .........: ${enable_lib_in_jar}")
else()
  message("        Compile Java ................: NO")
endif()
message("        Compile Lua .................: ${HAVE_LUA}")
message("        Compile Smpi ................: ${HAVE_SMPI}")
message("          Smpi fortran ..............: ${SMPI_FORTRAN}")
message("          MPICH3 testsuite ..........: ${enable_smpi_MPICH3_testsuite}")
message("          Privatization .............: ${HAVE_PRIVATIZATION}")
message("        Compile Boost.Context support: ${HAVE_BOOST_CONTEXTS}")
message("")
message("        Maintainer mode .............: ${enable_maintainer_mode}")
message("        Documentation................: ${enable_documentation}")
message("        Model checking ..............: ${HAVE_MC}")
message("        Jedule  mode ................: ${HAVE_JEDULE}")
message("        Graphviz mode ...............: ${HAVE_GRAPHVIZ}")
message("        Sigc++ mode .................: ${SIMGRID_HAVE_LIBSIG}")
message("        Mallocators .................: ${enable_mallocators}")
message("")
message("        Simgrid dependencies ........: ${SIMGRID_DEP}")
message("")

execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/Testing/Notes/)
file(WRITE ${PROJECT_BINARY_DIR}/Testing/Notes/Build  "GIT version : ${GIT_VERSION}\n")
file(APPEND ${PROJECT_BINARY_DIR}/Testing/Notes/Build "Release     : simgrid-${release_version}\n")

INCLUDE(Dart)
