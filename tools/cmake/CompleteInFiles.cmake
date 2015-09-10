set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_HOME_DIRECTORY}/tools/cmake/Modules
  )

# x86_64
# x86
# i.86

### Determine the assembly flavor that we need today
include(CMakeDetermineSystem)
IF(CMAKE_SYSTEM_PROCESSOR MATCHES ".86|AMD64|amd64")
  IF(${ARCH_32_BITS})
    message(STATUS "System processor: i686 (${CMAKE_SYSTEM_PROCESSOR}, 32 bits)")
    set(PROCESSOR_i686 1)
  ELSE()
    message(STATUS "System processor: x86_64 (${CMAKE_SYSTEM_PROCESSOR}, 64 bits)")
    set(PROCESSOR_x86_64 1)
  ENDIF()
  set(HAVE_RAWCTX 1)

ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^alpha")
  message(STATUS "System processor: alpha")

ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
  # Subdir is "arm" for both big-endian (arm) and little-endian (armel).
  message(STATUS "System processor: arm")

ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^mips")
  # mips* machines are bi-endian mostly so processor does not tell
  # endianess of the underlying system.
  message(STATUS "System processor: ${CMAKE_SYSTEM_PROCESSOR}" "mips" "mipsel" "mipseb")

ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64")
  message(STATUS "System processor: ppc64")

ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)")
  message(STATUS "System processor: ppc")

ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^sparc")
  # Both flavours can run on the same processor
  message(STATUS "System processor: ${CMAKE_SYSTEM_PROCESSOR}" "sparc" "sparcv9")

ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^(parisc|hppa)")
  message(STATUS "System processor: parisc" "parisc64")

ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^s390")
  # s390 binaries can run on s390x machines
  message(STATUS "System processor: ${CMAKE_SYSTEM_PROCESSOR}" "s390" "s390x")

ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^sh")
  message(STATUS "System processor: sh")

ELSE() #PROCESSOR NOT FOUND
  message(STATUS "PROCESSOR NOT FOUND: ${CMAKE_SYSTEM_PROCESSOR}")

ENDIF()

if(ARCH_32_BITS)
  set(MPI_ADDRESS_SIZE 4)
else()
  set(MPI_ADDRESS_SIZE 8)
endif()

message(STATUS "Cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")

include(CheckFunctionExists)
include(CheckTypeSize)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(TestBigEndian)
TEST_BIG_ENDIAN(BIGENDIAN)

include(FindGraphviz)
include(FindLibSigc++)

if(enable_java)
  find_package(Java REQUIRED COMPONENTS Runtime Development)
  find_package(JNI REQUIRED)
  message("-- [Java] JNI found: ${JNI_FOUND}")
  message("-- [Java] JNI include dirs: ${JNI_INCLUDE_DIRS}")
  if(enable_maintainer_mode)
    find_package(SWIG REQUIRED)
    include(UseSWIG)
    message("-- [Java] Swig found: ${SWIG_FOUND} (version ${SWIG_VERSION})")
  endif()
  set(HAVE_Java 1)
endif()
if(enable_scala)
  find_package(Scala REQUIRED)
  message("-- [Scala] scalac found: ${SCALA_COMPILE}")
  set(HAVE_Scala 1)
endif()
if(enable_lua)
  include(FindLua51Simgrid)
endif()

set(HAVE_NS3 0)
if(enable_ns3)
  include(FindNS3)
  if (NOT HAVE_NS3)
    message(FATAL_ERROR "Cannot find NS3. Please install it (apt-get install ns3 libns3-dev) or disable that cmake option")
  endif()
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
                        "(libboost-context-dev is optional)")
  endif()
endif()

# Try again to see if we have libboost-context
find_package(Boost 1.42 COMPONENTS context)
set(Boost_FOUND 1) # We don't care of whether this component is missing

if(Boost_FOUND AND Boost_CONTEXT_FOUND)
  # We should use feature detection for this instead:
  if (Boost_VERSION LESS 105600)
    message("Found Boost.Context API v1")
    set(HAVE_BOOST_CONTEXT 1)
  else()
    message("Found Boost.Context API v2")
    set(HAVE_BOOST_CONTEXT 2)
  endif()
else()
  message ("   boost        : found.")
  message ("   boost-context: missing. Install libboost-context-dev for this optional feature.")
  set(HAVE_BOOST_CONTEXT 0)
endif()

# Checks for header libraries functions.
CHECK_LIBRARY_EXISTS(dl      dlopen                  "" HAVE_DLOPEN_IN_LIBDL)
CHECK_LIBRARY_EXISTS(execinfo backtrace              "" HAVE_BACKTRACE_IN_LIBEXECINFO)
CHECK_LIBRARY_EXISTS(pthread pthread_create          "" HAVE_PTHREAD)
CHECK_LIBRARY_EXISTS(pthread sem_init                "" HAVE_SEM_INIT_LIB)
CHECK_LIBRARY_EXISTS(pthread sem_open                "" HAVE_SEM_OPEN_LIB)
CHECK_LIBRARY_EXISTS(pthread sem_timedwait           "" HAVE_SEM_TIMEDWAIT_LIB)
CHECK_LIBRARY_EXISTS(pthread pthread_mutex_timedlock "" HAVE_MUTEX_TIMEDLOCK_LIB)
CHECK_LIBRARY_EXISTS(rt      clock_gettime           "" HAVE_POSIX_GETTIME)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(CMAKE_REQUIRED_DEFINITIONS "-D_XOPEN_SOURCE=700 -D_DARWIN_C_SOURCE")
endif()

if(MINGW)
  add_definitions(-D__USE_MINGW_ANSI_STDIO=1)
  set(CMAKE_REQUIRED_DEFINITIONS "-D__USE_MINGW_ANSI_STDIO=1")
endif()

CHECK_INCLUDE_FILES("time.h;sys/time.h" TIME_WITH_SYS_TIME)
CHECK_INCLUDE_FILES("stdlib.h;stdarg.h;string.h;float.h" STDC_HEADERS)
CHECK_INCLUDE_FILE("pthread.h" HAVE_PTHREAD_H)
CHECK_INCLUDE_FILE("valgrind/valgrind.h" HAVE_VALGRIND_VALGRIND_H)
CHECK_INCLUDE_FILE("socket.h" HAVE_SOCKET_H)
CHECK_INCLUDE_FILE("sys/socket.h" HAVE_SYS_SOCKET_H)
CHECK_INCLUDE_FILE("stat.h" HAVE_STAT_H)
CHECK_INCLUDE_FILE("sys/stat.h" HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE("windows.h" HAVE_WINDOWS_H)
CHECK_INCLUDE_FILE("winsock.h" HAVE_WINSOCK_H)
CHECK_INCLUDE_FILE("winsock2.h" HAVE_WINSOCK2_H)
CHECK_INCLUDE_FILE("windef.h" HAVE_WINDEF_H)
CHECK_INCLUDE_FILE("errno.h" HAVE_ERRNO_H)
CHECK_INCLUDE_FILE("unistd.h" HAVE_UNISTD_H)
CHECK_INCLUDE_FILE("execinfo.h" HAVE_EXECINFO_H)
CHECK_INCLUDE_FILE("signal.h" HAVE_SIGNAL_H)
CHECK_INCLUDE_FILE("sys/time.h" HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILE("sys/param.h" HAVE_SYS_PARAM_H)
CHECK_INCLUDE_FILE("sys/sysctl.h" HAVE_SYS_SYSCTL_H)
CHECK_INCLUDE_FILE("time.h" HAVE_TIME_H)
CHECK_INCLUDE_FILE("inttypes.h" HAVE_INTTYPES_H)
CHECK_INCLUDE_FILE("memory.h" HAVE_MEMORY_H)
CHECK_INCLUDE_FILE("stdlib.h" HAVE_STDLIB_H)
CHECK_INCLUDE_FILE("strings.h" HAVE_STRINGS_H)
CHECK_INCLUDE_FILE("string.h" HAVE_STRING_H)
CHECK_INCLUDE_FILE("ucontext.h" HAVE_UCONTEXT_H)
CHECK_INCLUDE_FILE("stdio.h" HAVE_STDIO_H)
CHECK_INCLUDE_FILE("linux/futex.h" HAVE_FUTEX_H)

CHECK_FUNCTION_EXISTS(gettimeofday HAVE_GETTIMEOFDAY)
CHECK_FUNCTION_EXISTS(nanosleep HAVE_NANOSLEEP)
CHECK_FUNCTION_EXISTS(getdtablesize HAVE_GETDTABLESIZE)
CHECK_FUNCTION_EXISTS(sysconf HAVE_SYSCONF)
CHECK_FUNCTION_EXISTS(readv HAVE_READV)
CHECK_FUNCTION_EXISTS(popen HAVE_POPEN)
CHECK_FUNCTION_EXISTS(signal HAVE_SIGNAL)

CHECK_SYMBOL_EXISTS(snprintf stdio.h HAVE_SNPRINTF)
CHECK_SYMBOL_EXISTS(vsnprintf stdio.h HAVE_VSNPRINTF)
CHECK_SYMBOL_EXISTS(asprintf stdio.h HAVE_ASPRINTF)
CHECK_SYMBOL_EXISTS(vasprintf stdio.h HAVE_VASPRINTF)

CHECK_FUNCTION_EXISTS(makecontext HAVE_MAKECONTEXT)
CHECK_FUNCTION_EXISTS(mmap HAVE_MMAP)
CHECK_FUNCTION_EXISTS(process_vm_readv HAVE_PROCESS_VM_READV)
CHECK_FUNCTION_EXISTS(strdup SIMGRID_HAVE_STRDUP)
CHECK_FUNCTION_EXISTS(_strdup SIMGRID_HAVE__STRDUP)

#Check if __thread is defined
execute_process(
  COMMAND "${CMAKE_C_COMPILER} ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_thread_storage.c"
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  RESULT_VARIABLE HAVE_thread_storage_run
  )

if(HAVE_thread_storage_run)
  set(HAVE_THREAD_LOCAL_STORAGE 1)
else()
  set(HAVE_THREAD_LOCAL_STORAGE 0)
endif()

# Our usage of mmap is Linux-specific (flag MAP_ANONYMOUS), but kFreeBSD uses a GNU libc
IF(NOT "${CMAKE_SYSTEM}" MATCHES "Linux" AND NOT "${CMAKE_SYSTEM}" MATCHES "kFreeBSD" AND NOT "${CMAKE_SYSTEM}" MATCHES "GNU" AND NOT  "${CMAKE_SYSTEM}" MATCHES "Darwin")
  SET(HAVE_MMAP 0)
  message(STATUS "Warning: MMAP is thought as non functional on this architecture (${CMAKE_SYSTEM})")
ENDIF()

if(HAVE_MMAP AND HAVE_THREAD_LOCAL_STORAGE)
  SET(HAVE_MMALLOC 1)
else()
  SET(HAVE_MMALLOC 0)
endif()


if(WIN32) #THOSE FILES ARE FUNCTIONS ARE NOT DETECTED BUT THEY SHOULD...
  set(HAVE_UCONTEXT_H 1)
  set(HAVE_MAKECONTEXT 1)
  set(HAVE_SNPRINTF 1)
  set(HAVE_VSNPRINTF 1)
endif()

set(CONTEXT_UCONTEXT 0)
SET(CONTEXT_THREADS 0)

if(enable_jedule)
  SET(HAVE_JEDULE 1)
endif()

if(enable_latency_bound_tracking)
  SET(HAVE_LATENCY_BOUND_TRACKING 1)
else()
  SET(HAVE_LATENCY_BOUND_TRACKING 0)
endif()

if(enable_mallocators)
  SET(MALLOCATOR_IS_WANTED 1)
else()
  SET(MALLOCATOR_IS_WANTED 0)
endif()

if(enable_model-checking AND HAVE_MMALLOC)
  SET(HAVE_MC 1)
  SET(MMALLOC_WANT_OVERRIDE_LEGACY 1)
  include(FindLibunwind)
  include(FindLibdw)
else()
  if(enable_model-checking)
    message(STATUS "Warning: support for model-checking has been disabled because HAVE_MMALLOC is false")
  endif()
  SET(HAVE_MC 0)
  SET(HAVE_MMALLOC 0)
  SET(MMALLOC_WANT_OVERRIDE_LEGACY 0)
endif()

if(enable_smpi)
  include(FindGFortran)
  #really checks for objdump for privatization
  find_package(BinUtils QUIET)
  SET(HAVE_SMPI 1)

  if( NOT "${CMAKE_OBJDUMP}" MATCHES "CMAKE_OBJDUMP-NOTFOUND" AND HAVE_MMAP)
    SET(HAVE_PRIVATIZATION 1)
  else()
    SET(HAVE_PRIVATIZATION 0)
  endif()
endif()

#--------------------------------------------------------------------------------------------------
### Check for some architecture dependent values
CHECK_TYPE_SIZE(int SIZEOF_INT)
CHECK_TYPE_SIZE(void* SIZEOF_VOIDP)

#--------------------------------------------------------------------------------------------------
### Check for GNU dynamic linker
CHECK_INCLUDE_FILE("dlfcn.h" HAVE_DLFCN_H)
if (HAVE_DLFCN_H)
    if(HAVE_DLOPEN_IN_LIBDL)
      set(DL_LIBRARY "-ldl")
    endif(HAVE_DLOPEN_IN_LIBDL)
    execute_process(COMMAND ${CMAKE_C_COMPILER} ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_gnu_dynlinker.c ${DL_LIBRARY} -o test_gnu_ld
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

if(HAVE_PTHREAD)
  set(pthread 1)
elseif(pthread)
  set(pthread 0)
endif()

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
    RESULT_VARIABLE HAVE_SEM_INIT_run OUTPUT_VARIABLE HAVE_SEM_INIT_compil)

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

  ### Test that we have a way to timewait for semaphores

  if(HAVE_SEM_TIMEDWAIT_LIB)

    execute_process(
      COMMAND "${CMAKE_C_COMPILER} ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_sem_timedwait.c -lpthread"
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      OUTPUT_VARIABLE HAVE_SEM_TIMEDWAIT_run
      )

    if(HAVE_SEM_TIMEDWAIT_run)
      set(HAVE_SEM_TIMEDWAIT 0)
      message(STATUS "timedwait not compilable")
    else()
      set(HAVE_SEM_TIMEDWAIT 1)
      message(STATUS "timedwait is compilable")
    endif()
  endif()

  ### HAVE_MUTEX_TIMEDLOCK

  if(HAVE_MUTEX_TIMEDLOCK_LIB)

    execute_process(
      COMMAND "${CMAKE_C_COMPILER} ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_mutex_timedlock.c -lpthread"
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      OUTPUT_VARIABLE HAVE_MUTEX_TIMEDLOCK_run
      )

    if(HAVE_MUTEX_TIMEDLOCK_run)
      set(HAVE_MUTEX_TIMEDLOCK 0)
      message(STATUS "timedlock not compilable")
    else()
      message(STATUS "timedlock is compilable")
      set(HAVE_MUTEX_TIMEDLOCK 1)
    endif()
  endif()
endif()

# This is needed for ucontext on MacOS X:
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  add_definitions(-D_XOPEN_SOURCE=700 -D_DARWIN_C_SOURCE)
endif()

if(WIN32)
  # We always provide our own implementation of ucontext on Windows.
  try_compile(HAVE_UCONTEXT
    ${CMAKE_BINARY_DIR}
    ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_AC_CHECK_MCSC.c
    COMPILE_DEFINITIONS _XBT_WIN32
    INCLUDE_DIRECTORIES
      ${CMAKE_HOME_DIRECTORY}/src/include
      ${CMAKE_HOME_DIRECTORY}/src/xbt
  )
else()
  # We always provide our own implementation of ucontext on Windows.
  try_compile(HAVE_UCONTEXT
    ${CMAKE_BINARY_DIR}
    ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_AC_CHECK_MCSC.c)
endif()

#If can have both context

if(HAVE_UCONTEXT)
  set(CONTEXT_UCONTEXT 1)
  message("-- Support for ucontext factory")
endif()

if(HAVE_PTHREAD)
  set(CONTEXT_THREADS 1)
  message("-- Support for thread context factory")
endif()

###############
## GIT version check
##
if(EXISTS ${CMAKE_HOME_DIRECTORY}/.git/)
  execute_process(COMMAND git remote
  COMMAND head -n 1
  WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/.git/
  OUTPUT_VARIABLE remote
  RESULT_VARIABLE ret
  )
  string(REPLACE "\n" "" remote "${remote}")
  #message(STATUS "Git remote: ${remote}")
  execute_process(COMMAND git config --get remote.${remote}.url
  WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/.git/
  OUTPUT_VARIABLE url
  RESULT_VARIABLE ret
  )
  string(REPLACE "\n" "" url "${url}")
  #message(STATUS "Git url: ${url}")
  if(url)
    execute_process(COMMAND git --git-dir=${CMAKE_HOME_DIRECTORY}/.git log --pretty=oneline --abbrev-commit -1
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/.git/
    OUTPUT_VARIABLE GIT_VERSION
    RESULT_VARIABLE ret
    )
    string(REPLACE "\n" "" GIT_VERSION "${GIT_VERSION}")
    message(STATUS "Git version: ${GIT_VERSION}")
    execute_process(COMMAND git --git-dir=${CMAKE_HOME_DIRECTORY}/.git log -n 1 --pretty=format:%ai .
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/.git/
    OUTPUT_VARIABLE GIT_DATE
    RESULT_VARIABLE ret
    )
    string(REPLACE "\n" "" GIT_DATE "${GIT_DATE}")
    message(STATUS "Git date: ${GIT_DATE}")
    string(REGEX REPLACE " .*" "" GIT_VERSION "${GIT_VERSION}")
    
    execute_process(COMMAND git --git-dir=${CMAKE_HOME_DIRECTORY}/.git log --pretty=format:%H -1
                    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/.git/
		    OUTPUT_VARIABLE SIMGRID_GITHASH
		    RESULT_VARIABLE ret
		    )
    string(REPLACE "\n" "" SIMGRID_GITHASH "${SIMGRID_GITHASH}")
		    
  endif()
elseif(EXISTS ${CMAKE_HOME_DIRECTORY}/.gitversion)
  FILE(STRINGS ${CMAKE_HOME_DIRECTORY}/.gitversion GIT_VERSION)
endif()

if(release)
  set(SIMGRID_VERSION_BANNER "${SIMGRID_VERSION_BANNER}\\nRelease build")
else()
  set(SIMGRID_VERSION_BANNER "${SIMGRID_VERSION_BANNER}\\nDevelopment build")
endif()
if(GIT_VERSION)
  set(SIMGRID_VERSION_BANNER "${SIMGRID_VERSION_BANNER} at commit ${GIT_VERSION}")
endif()
if(GIT_DATE)
  set(SIMGRID_VERSION_BANNER "${SIMGRID_VERSION_BANNER} (${GIT_DATE})")
endif()
#--------------------------------------------------------------------------------------------------

set(makecontext_CPPFLAGS_2 "")
if(HAVE_MAKECONTEXT OR WIN32)
  set(makecontext_CPPFLAGS "-DTEST_makecontext")
  if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(makecontext_CPPFLAGS_2 "-D_XOPEN_SOURCE=700")
  endif()

  if(WIN32)
    if(ARCH_32_BITS)
      set(makecontext_CPPFLAGS "-DTEST_makecontext -D_I_X86_")
    else()
      set(makecontext_CPPFLAGS "-DTEST_makecontext -D_AMD64_")
    endif()
    set(makecontext_CPPFLAGS_2 "-D_XBT_WIN32 -I${CMAKE_HOME_DIRECTORY}/src/include -I${CMAKE_HOME_DIRECTORY}/src/xbt")
  endif()

  file(REMOVE ${CMAKE_BINARY_DIR}/conftestval)

  if(CMAKE_CROSSCOMPILING)
    set(RUN_makecontext_VAR "cross")
    set(COMPILE_makecontext_VAR "cross")
  else()
    try_run(RUN_makecontext_VAR COMPILE_makecontext_VAR
      ${CMAKE_BINARY_DIR}
      ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_stacksetup.c
      COMPILE_DEFINITIONS "${makecontext_CPPFLAGS} ${makecontext_CPPFLAGS_2}"
      )
  endif()

  if(EXISTS ${CMAKE_BINARY_DIR}/conftestval)
    file(READ ${CMAKE_BINARY_DIR}/conftestval MAKECONTEXT_ADDR_SIZE)
    string(REPLACE "\n" "" MAKECONTEXT_ADDR_SIZE "${MAKECONTEXT_ADDR_SIZE}")
    string(REGEX MATCH ;^.*,;MAKECONTEXT_ADDR "${MAKECONTEXT_ADDR_SIZE}")
    string(REGEX MATCH ;,.*$; MAKECONTEXT_SIZE "${MAKECONTEXT_ADDR_SIZE}")
    string(REPLACE "," "" makecontext_addr "${MAKECONTEXT_ADDR}")
    string(REPLACE "," "" makecontext_size "${MAKECONTEXT_SIZE}")
    set(pth_skaddr_makecontext "#define pth_skaddr_makecontext(skaddr,sksize) (${makecontext_addr})")
    set(pth_sksize_makecontext "#define pth_sksize_makecontext(skaddr,sksize) (${makecontext_size})")
    message(STATUS "${pth_skaddr_makecontext}")
    message(STATUS "${pth_sksize_makecontext}")
  else()
    # message(FATAL_ERROR "makecontext is not compilable")
  endif()
endif()

#--------------------------------------------------------------------------------------------------

### check for stackgrowth
if (NOT CMAKE_CROSSCOMPILING)
  try_run(RUN_makecontext_VAR COMPILE_makecontext_VAR
    ${CMAKE_BINARY_DIR}
    ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_stackgrowth.c
    RUN_OUTPUT_VARIABLE stack
    )
endif()
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
    message(ERROR "Could not figure the stack direction.")
  endif()
endif()

###############
## System checks
##

#SG_CONFIGURE_PART([System checks...])
#AC_PROG_CC(xlC gcc cc) -auto
#AM_SANITY_CHECK -auto

#AC_PROG_MAKE_SET

#AC_CHECK_VA_COPY

set(diff_va "va_copy((d),(s))"
  "VA_COPY((d),(s))"
  "__va_copy((d),(s))"
  "__builtin_va_copy((d),(s))"
  "do { (d) = (s)\; } while (0)"
  "do { *(d) = *(s)\; } while (0)"
  "memcpy((void *)&(d), (void *)&(s), sizeof(s))"
  "memcpy((void *)(d), (void *)(s), sizeof(*(s)))"
  )

foreach(fct ${diff_va})
  write_file("${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_va_copy.c" "#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#define DO_VA_COPY(d,s) ${fct}
void test(char *str, ...)
{
  va_list ap, ap2;
  int i;
  va_start(ap, str);
  DO_VA_COPY(ap2, ap);
  for (i = 1; i <= 9; i++) {
    int k = (int)va_arg(ap, int);
    if (k != i)
      abort();
  }
  DO_VA_COPY(ap, ap2);
  for (i = 1; i <= 9; i++) {
    int k = (int)va_arg(ap, int);
    if (k != i)
      abort();
  }
  va_end(ap);
}
int main(void)
{
  test(\"test\", 1, 2, 3, 4, 5, 6, 7, 8, 9);
  exit(0);
}"
    )

  execute_process(
  COMMAND ${CMAKE_C_COMPILER} "${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_va_copy.c"
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  RESULT_VARIABLE COMPILE_VA_NULL_VAR
  OUTPUT_QUIET
  ERROR_QUIET
  )

  if(NOT COMPILE_VA_NULL_VAR)
    string(REGEX REPLACE "\;" "" fctbis ${fct})
    if(${fctbis} STREQUAL "va_copy((d),(s))")
      set(HAVE_VA_COPY 1)
      set(ac_cv_va_copy "C99")
      set(__VA_COPY_USE_C99 "va_copy((d),(s))")
    endif()

    if(${fctbis} STREQUAL "VA_COPY((d),(s))")
      set(ac_cv_va_copy "GCM")
      set(__VA_COPY_USE_GCM "VA_COPY((d),(s))")
    endif()

    if(${fctbis} STREQUAL "__va_copy((d),(s))")
      set(ac_cv_va_copy "GCH")
      set(__VA_COPY_USE_GCH "__va_copy((d),(s))")
    endif()

    if(${fctbis} STREQUAL "__builtin_va_copy((d),(s))")
      set(ac_cv_va_copy "GCB")
      set(__VA_COPY_USE_GCB "__builtin_va_copy((d),(s))")
    endif()

    if(${fctbis} STREQUAL "do { (d) = (s) } while (0)")
      set(ac_cv_va_copy "ASS")
      set(__VA_COPY_USE_ASS "do { (d) = (s); } while (0)")
    endif()

    if(${fctbis} STREQUAL "do { *(d) = *(s) } while (0)")
      set(ac_cv_va_copy "ASP")
      set(__VA_COPY_USE_ASP "do { *(d) = *(s); } while (0)")
    endif()

    if(${fctbis} STREQUAL "memcpy((void *)&(d), (void *)&(s), sizeof(s))")
      set(ac_cv_va_copy "CPS")
      set(__VA_COPY_USE_CPS "memcpy((void *)&(d), (void *)&(s), sizeof(s))")
    endif()

    if(${fctbis} STREQUAL "memcpy((void *)(d), (void *)(s), sizeof(*(s)))")
      set(ac_cv_va_copy "CPP")
      set(__VA_COPY_USE_CPP "memcpy((void *)(d), (void *)(s), sizeof(*(s)))")
    endif()

    if(NOT STATUS_OK)
      set(__VA_COPY_USE "__VA_COPY_USE_${ac_cv_va_copy}(d, s)")
    endif()
    set(STATUS_OK "1")

  endif()

endforeach(fct ${diff_va})

#--------------------------------------------------------------------------------------------------
### check for a working snprintf
if(HAVE_SNPRINTF AND HAVE_VSNPRINTF OR WIN32)
  if(WIN32)
    #set(HAVE_SNPRINTF 1)
    #set(HAVE_VSNPRINTF 1)
  endif()

  if(CMAKE_CROSSCOMPILING)
    set(RUN_SNPRINTF_FUNC "cross")
    #set(PREFER_PORTABLE_SNPRINTF 1)
  else()
    try_run(RUN_SNPRINTF_FUNC_VAR COMPILE_SNPRINTF_FUNC_VAR
      ${CMAKE_BINARY_DIR}
      ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_snprintf.c
      )
  endif()

  if(CMAKE_CROSSCOMPILING)
    set(RUN_VSNPRINTF_FUNC "cross")
    set(PREFER_PORTABLE_VSNPRINTF 1)
  else()
    try_run(RUN_VSNPRINTF_FUNC_VAR COMPILE_VSNPRINTF_FUNC_VAR
      ${CMAKE_BINARY_DIR}
      ${CMAKE_HOME_DIRECTORY}/tools/cmake/test_prog/prog_vsnprintf.c
      )
  endif()

  set(PREFER_PORTABLE_SNPRINTF 0)
  if(RUN_VSNPRINTF_FUNC_VAR MATCHES "FAILED_TO_RUN")
    set(PREFER_PORTABLE_SNPRINTF 1)
  endif()
  if(RUN_SNPRINTF_FUNC_VAR MATCHES "FAILED_TO_RUN")
    set(PREFER_PORTABLE_SNPRINTF 1)
  endif()
endif()

### check for asprintf function familly
if(HAVE_ASPRINTF)
  SET(simgrid_need_asprintf "")
  SET(NEED_ASPRINTF 0)
else()
  SET(simgrid_need_asprintf "#define SIMGRID_NEED_ASPRINTF 1")
  SET(NEED_ASPRINTF 1)
endif()

if(HAVE_VASPRINTF)
  SET(simgrid_need_vasprintf "")
  SET(NEED_VASPRINTF 0)
else()
  SET(simgrid_need_vasprintf "#define SIMGRID_NEED_VASPRINTF 1")
  SET(NEED_VASPRINTF 1)
endif()

### check for addr2line

find_path(ADDR2LINE NAMES addr2line	PATHS NO_DEFAULT_PATHS	)
if(ADDR2LINE)
  set(ADDR2LINE "${ADDR2LINE}/addr2line")
endif()

### File to create

configure_file("${CMAKE_HOME_DIRECTORY}/src/context_sysv_config.h.in"
  "${CMAKE_BINARY_DIR}/src/context_sysv_config.h" @ONLY IMMEDIATE)

SET( CMAKEDEFINE "#cmakedefine" )
configure_file("${CMAKE_HOME_DIRECTORY}/tools/cmake/src/internal_config.h.in" "${CMAKE_BINARY_DIR}/src/internal_config.h" @ONLY IMMEDIATE)
configure_file("${CMAKE_BINARY_DIR}/src/internal_config.h" "${CMAKE_BINARY_DIR}/src/internal_config.h" @ONLY IMMEDIATE)
configure_file("${CMAKE_HOME_DIRECTORY}/include/simgrid_config.h.in" "${CMAKE_BINARY_DIR}/include/simgrid_config.h" @ONLY IMMEDIATE)

set(top_srcdir "${CMAKE_HOME_DIRECTORY}")
set(srcdir "${CMAKE_HOME_DIRECTORY}/src")
set(bindir "${CMAKE_BINARY_DIR}")

### Script used when simgrid is installed
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(includeflag "-I${CMAKE_INSTALL_PREFIX}/include -I${CMAKE_INSTALL_PREFIX}/include/smpi")
set(includedir "${CMAKE_INSTALL_PREFIX}/include")
set(libdir ${exec_prefix}/lib)
set(CMAKE_SMPI_COMMAND "export LD_LIBRARY_PATH=\"${CMAKE_INSTALL_PREFIX}/lib")
if(NS3_LIBRARY_PATH)
  set(CMAKE_SMPI_COMMAND "${CMAKE_SMPI_COMMAND}:${NS3_LIBRARY_PATH}")
endif()
set(CMAKE_SMPI_COMMAND "${CMAKE_SMPI_COMMAND}:\${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}\"")

file(READ ${CMAKE_HOME_DIRECTORY}/src/smpi/smpitools.sh SMPITOOLS_SH)
configure_file(${CMAKE_HOME_DIRECTORY}/include/smpi/mpif.h.in ${CMAKE_BINARY_DIR}/include/smpi/mpif.h @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpicc.in ${CMAKE_BINARY_DIR}/bin/smpicc @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpicxx.in ${CMAKE_BINARY_DIR}/bin/smpicxx @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpiff.in ${CMAKE_BINARY_DIR}/bin/smpiff @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpif90.in ${CMAKE_BINARY_DIR}/bin/smpif90 @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpirun.in ${CMAKE_BINARY_DIR}/bin/smpirun @ONLY)

### Script used when simgrid is compiling
set(includeflag "-I${CMAKE_HOME_DIRECTORY}/include -I${CMAKE_HOME_DIRECTORY}/include/smpi")
set(includeflag "${includeflag} -I${CMAKE_BINARY_DIR}/include -I${CMAKE_BINARY_DIR}/include/smpi")
set(includedir "${CMAKE_HOME_DIRECTORY}/include")
set(exec_prefix "${CMAKE_BINARY_DIR}/smpi_script/")
set(CMAKE_SMPI_COMMAND "export LD_LIBRARY_PATH=\"${CMAKE_BINARY_DIR}/lib")
if(NS3_LIBRARY_PATH)
  set(CMAKE_SMPI_COMMAND "${CMAKE_SMPI_COMMAND}:${NS3_LIBRARY_PATH}")
endif()
set(CMAKE_SMPI_COMMAND "${CMAKE_SMPI_COMMAND}:\${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}\"")
set(libdir "${CMAKE_BINARY_DIR}/lib")

configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpicc.in ${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpicxx.in ${CMAKE_BINARY_DIR}/smpi_script/bin/smpicxx @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpiff.in ${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpif90.in ${CMAKE_BINARY_DIR}/smpi_script/bin/smpif90 @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpirun.in ${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun @ONLY)

set(top_builddir ${CMAKE_HOME_DIRECTORY})

if(NOT WIN32)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpicc)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpicxx)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpiff)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpif90)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpirun)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/smpi_script/bin/smpicxx)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/smpi_script/bin/smpif90)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun)
endif()

set(generated_headers_to_install
  ${CMAKE_CURRENT_BINARY_DIR}/include/smpi/mpif.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/simgrid_config.h
  )

set(generated_headers
  ${CMAKE_CURRENT_BINARY_DIR}/src/context_sysv_config.h
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

IF(${ARCH_32_BITS})
  set(WIN_ARCH "32")
ELSE()
  set(WIN_ARCH "64")
ENDIF()
configure_file("${CMAKE_HOME_DIRECTORY}/tools/cmake/src/simgrid.nsi.in" "${CMAKE_BINARY_DIR}/simgrid.nsi" @ONLY IMMEDIATE)
