set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/Modules
  )

# x86_64
# x86
# i.86

IF(CMAKE_SYSTEM_PROCESSOR MATCHES ".86")
  IF(${ARCH_32_BITS})
    set(PROCESSOR_i686 1)
    set(SIMGRID_SYSTEM_PROCESSOR "${CMAKE_SYSTEM_PROCESSOR}")
    message(STATUS "System processor: ${CMAKE_SYSTEM_PROCESSOR}")
  ELSE()
    message(STATUS "System processor: amd64")
    set(SIMGRID_SYSTEM_PROCESSOR "amd64")
    set(PROCESSOR_x86_64 1)
    set(PROCESSOR_i686 0)
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

message(STATUS "Cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")

include(CheckFunctionExists)
include(CheckTypeSize)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(TestBigEndian)
TEST_BIG_ENDIAN(BIGENDIAN)

include(FindGraphviz)
if(WIN32)
  include(FindPcreWin)
else()
  include(FindPCRE)
endif()

set(HAVE_GTNETS 0)
if(enable_gtnets)
  include(FindGTnets)
endif()
if(enable_smpi)
  include(FindF2c)
  SET(HAVE_SMPI 1)
endif()
if(enable_lua)
  include(FindLua51Simgrid)
endif()
set(HAVE_NS3 0)
if(enable_ns3)
  include(FindNS3)
endif()

# Checks for header libraries functions.
CHECK_LIBRARY_EXISTS(pthread 	pthread_create 			"" pthread)
CHECK_LIBRARY_EXISTS(pthread 	sem_init 				"" HAVE_SEM_INIT_LIB)
CHECK_LIBRARY_EXISTS(pthread 	sem_open 				"" HAVE_SEM_OPEN_LIB)
CHECK_LIBRARY_EXISTS(pthread 	sem_timedwait 			"" HAVE_SEM_TIMEDWAIT_LIB)
CHECK_LIBRARY_EXISTS(pthread 	pthread_mutex_timedlock "" HAVE_MUTEX_TIMEDLOCK_LIB)
CHECK_LIBRARY_EXISTS(rt 		clock_gettime 			"" HAVE_POSIX_GETTIME)

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
CHECK_INCLUDE_FILE("WinDef.h" HAVE_WINDEF_H)
CHECK_INCLUDE_FILE("errno.h" HAVE_ERRNO_H)
CHECK_INCLUDE_FILE("unistd.h" HAVE_UNISTD_H)
CHECK_INCLUDE_FILE("execinfo.h" HAVE_EXECINFO_H)
CHECK_INCLUDE_FILE("signal.h" HAVE_SIGNAL_H)
CHECK_INCLUDE_FILE("sys/time.h" HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILE("sys/param.h" HAVE_SYS_PARAM_H)
CHECK_INCLUDE_FILE("sys/sysctl.h" HAVE_SYS_SYSCTL_H)
CHECK_INCLUDE_FILE("time.h" HAVE_TIME_H)
CHECK_INCLUDE_FILE("dlfcn.h" HAVE_DLFCN_H)
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
CHECK_FUNCTION_EXISTS(snprintf HAVE_SNPRINTF)
CHECK_FUNCTION_EXISTS(vsnprintf HAVE_VSNPRINTF)
CHECK_FUNCTION_EXISTS(asprintf HAVE_ASPRINTF)
CHECK_FUNCTION_EXISTS(vasprintf HAVE_VASPRINTF)
CHECK_FUNCTION_EXISTS(makecontext HAVE_MAKECONTEXT)
CHECK_FUNCTION_EXISTS(mmap HAVE_MMAP)
CHECK_FUNCTION_EXISTS(mergesort HAVE_MERGESORT)

#Check if __thread is defined
execute_process(
  COMMAND "${CMAKE_C_COMPILER} ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_thread_storage.c"
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  RESULT_VARIABLE HAVE_thread_storage_run
  )

if(HAVE_thread_storage_run)
  set(HAVE_THREAD_LOCAL_STORAGE 0)
else()
  set(HAVE_THREAD_LOCAL_STORAGE 1)
endif()

# Our usage of mmap is Linux-specific (flag MAP_ANONYMOUS), but kFreeBSD uses a GNU libc
IF(NOT "${CMAKE_SYSTEM}" MATCHES "Linux" AND NOT "${CMAKE_SYSTEM}" MATCHES "kFreeBSD" AND NOT "${CMAKE_SYSTEM}" MATCHES "GNU")
  SET(HAVE_MMAP 0)
  message(STATUS "Warning: MMAP is thought as non functional on this architecture (${CMAKE_SYSTEM})")
ENDIF()

if(WIN32) #THOSE FILES ARE FUNCTIONS ARE NOT DETECTED BUT THEY SHOULD...
  set(HAVE_UCONTEXT_H 1)
  set(HAVE_MAKECONTEXT 1)
  set(HAVE_SNPRINTF 1)
  set(HAVE_VSNPRINTF 1)
endif()

set(CONTEXT_UCONTEXT 0)
SET(CONTEXT_THREADS 0)
SET(HAVE_TRACING 1)

if(enable_tracing)
  SET(HAVE_TRACING 1)
else()
  SET(HAVE_TRACING 0)
endif()

if(enable_jedule)
  SET(HAVE_JEDULE 1)
endif()

if(enable_latency_bound_tracking)
  SET(HAVE_LATENCY_BOUND_TRACKING 1)
else()
  if(enable_gtnets)
    message(STATUS "Warning : Turning latency_bound_tracking to ON because GTNeTs is ON")
    SET(enable_latency_bound_tracking ON)
    SET(HAVE_LATENCY_BOUND_TRACKING 1)
  else()
    SET(HAVE_LATENCY_BOUND_TRACKING 0)
  endif()
endif()

if(enable_mallocators)
  SET(MALLOCATOR_IS_WANTED 1)
else()
  SET(MALLOCATOR_IS_WANTED 0)
endif()

if(enable_model-checking AND HAVE_MMAP)
  SET(HAVE_MC 1)
  SET(MMALLOC_WANT_OVERRIDE_LEGACY 1)
  include(FindLibunwind)
else()
  SET(HAVE_MC 0)
  SET(MMALLOC_WANT_OVERRIDE_LEGACY 0)
endif()

#--------------------------------------------------------------------------------------------------
### Check for some architecture dependent values
CHECK_TYPE_SIZE(int SIZEOF_INT)
CHECK_TYPE_SIZE(void* SIZEOF_VOIDP)

#--------------------------------------------------------------------------------------------------
### Initialize of CONTEXT THREADS

if(pthread)
  set(pthread 1)
elseif(pthread)
  set(pthread 0)
endif()

if(pthread)
  ### Test that we have a way to create semaphores

  if(HAVE_SEM_OPEN_LIB)
    execute_process(COMMAND ${CMAKE_C_COMPILER} ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_sem_open.c -lpthread -o sem_open
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    OUTPUT_VARIABLE HAVE_SEM_OPEN_compil
    )

    if(HAVE_SEM_OPEN_compil)
      set(HAVE_SEM_OPEN 0)
      message(STATUS "Warning: sem_open not compilable")
      message(STATUS "HAVE_SEM_OPEN_comp_output: ${HAVE_SEM_OPEN_comp_output}")
    else()
      set(HAVE_SEM_OPEN 1)
      message(STATUS "sem_open is compilable")
    endif()

    execute_process(COMMAND ./sem_open
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR} 
    RESULT_VARIABLE HAVE_SEM_OPEN_run 
    OUTPUT_VARIABLE var_compil
    )
    file(REMOVE sem_open)

    if(NOT HAVE_SEM_OPEN_run)
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

  else()
    set(HAVE_SEM_OPEN 0)
  endif()

  if(HAVE_SEM_INIT_LIB)
    execute_process(COMMAND ${CMAKE_C_COMPILER} ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_sem_init.c -lpthread -o sem_init
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR} 
    RESULT_VARIABLE HAVE_SEM_INIT_run OUTPUT_VARIABLE HAVE_SEM_INIT_compil)
    
    if(HAVE_SEM_INIT_compil)
      set(HAVE_SEM_INIT 0)
      message(STATUS "Warning: sem_init not compilable")
      message(STATUS "HAVE_SEM_INIT_comp_output: ${HAVE_SEM_OPEN_comp_output}")
    else()
      set(HAVE_SEM_INIT 1)
      message(STATUS "sem_init is compilable")
    endif()
    execute_process(COMMAND ./sem_init 
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR} 
    RESULT_VARIABLE HAVE_SEM_INIT_run 
    OUTPUT_VARIABLE var_compil
    )
    file(REMOVE sem_init)


    if(NOT HAVE_SEM_INIT_run)
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

  if(NOT HAVE_SEM_OPEN AND NOT HAVE_SEM_INIT)
    message(FATAL_ERROR "Semaphores are not usable (neither sem_open nor sem_init is both compilable and executable), but they are mandatory to threads (you may need to mount /dev).")
  endif()

  ### Test that we have a way to timewait for semaphores

  if(HAVE_SEM_TIMEDWAIT_LIB)

    execute_process(
      COMMAND "${CMAKE_C_COMPILER} ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_sem_timedwait.c -lpthread"
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
      COMMAND "${CMAKE_C_COMPILER} ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_mutex_timedlock.c -lpthread"
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

# AC_CHECK_MCSC(mcsc=yes, mcsc=no)
set(mcsc_flags "")
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(mcsc_flags "-D_XOPEN_SOURCE")
endif()

if(WIN32)
  if(ARCH_32_BITS)
    set(mcsc_flags "-D_XBT_WIN32 -D_I_X86_ -I${CMAKE_HOME_DIRECTORY}/include/xbt -I${CMAKE_HOME_DIRECTORY}/src/xbt")
  else()
    set(mcsc_flags "-D_XBT_WIN32 -D_AMD64_ -I${CMAKE_HOME_DIRECTORY}/include/xbt -I${CMAKE_HOME_DIRECTORY}/src/xbt")
  endif()
endif()

IF(CMAKE_CROSSCOMPILING)
  IF(WIN32)
    set(windows_context "yes")
    set(IS_WINDOWS 1)
  ENDIF()
ELSE()
  file(REMOVE "${CMAKE_BINARY_DIR}/testprog*")
  file(REMOVE ${CMAKE_BINARY_DIR}/conftestval)
  execute_process(COMMAND ${CMAKE_C_COMPILER} ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_AC_CHECK_MCSC.c ${mcsc_flags} -o testprog
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/
  OUTPUT_VARIABLE COMPILE_mcsc_VAR)

  if(NOT COMPILE_mcsc_VAR)
    message(STATUS "prog_AC_CHECK_MCSC.c is compilable")
    execute_process(COMMAND ./testprog
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/
    OUTPUT_VARIABLE var_compil)
  else()
    message(STATUS "prog_AC_CHECK_MCSC.c is not compilable")
  endif()
  file(REMOVE "${CMAKE_BINARY_DIR}/testprog*")

  if(EXISTS "${CMAKE_BINARY_DIR}/conftestval")
    file(READ "${CMAKE_BINARY_DIR}/conftestval" mcsc)
    STRING(REPLACE "\n" "" mcsc "${mcsc}")
    if(mcsc)
      set(mcsc "yes")
      set(HAVE_UCONTEXT_H 1)
    else()
      set(mcsc "no")
    endif()
  else()
    set(mcsc "no")
  endif()

  message(STATUS "mcsc: ${mcsc}")
ENDIF()

if(mcsc MATCHES "no" AND pthread)
  if(HAVE_WINDOWS_H)
    set(windows_context "yes")
    set(IS_WINDOWS 1)
  elseif(HAVE_WINDOWS_H)
    message(FATAL_ERROR "no appropriate backend found")
  endif()
endif()

#Only windows

if(WIN32)
  if(NOT HAVE_WINDOWS_H)
    message(FATAL_ERROR "no appropriate backend found windows")
  endif()
endif()

if(windows_context MATCHES "yes")
  message(STATUS "Context change to windows")
endif()

#If can have both context

if(mcsc)
  set(CONTEXT_UCONTEXT 1)
endif()

if(pthread)
  set(CONTEXT_THREADS 1)
endif()

###############
## GIT version check
##
if(EXISTS ${CMAKE_HOME_DIRECTORY}/.git/ AND NOT WIN32)
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
    STRING(REPLACE " +0000" "" GIT_DATE "${GIT_DATE}")
    STRING(REPLACE " " "~" GIT_DATE "${GIT_DATE}")
    STRING(REPLACE ":" "-" GIT_DATE "${GIT_DATE}")
  endif()
endif()

#--------------------------------------------------------------------------------------------------

set(makecontext_CPPFLAGS_2 "")
if(HAVE_MAKECONTEXT OR WIN32)
  set(makecontext_CPPFLAGS "-DTEST_makecontext")
  if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(makecontext_CPPFLAGS_2 "-D_XOPEN_SOURCE")
  endif()

  if(WIN32)
    if(ARCH_32_BITS)
      set(makecontext_CPPFLAGS "-DTEST_makecontext -D_I_X86_")
    else()
      set(makecontext_CPPFLAGS "-DTEST_makecontext -D_AMD64_")
    endif()
    set(makecontext_CPPFLAGS_2 "-D_XBT_WIN32 -I${CMAKE_HOME_DIRECTORY}/include/xbt -I${CMAKE_HOME_DIRECTORY}/src/xbt")
  endif()

  file(REMOVE ${CMAKE_BINARY_DIR}/conftestval)

  try_run(RUN_makecontext_VAR COMPILE_makecontext_VAR
    ${CMAKE_BINARY_DIR}
    ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_stacksetup.c
    COMPILE_DEFINITIONS "${makecontext_CPPFLAGS} ${makecontext_CPPFLAGS_2}"
    )

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
    #	    message(FATAL_ERROR "makecontext is not compilable")
  endif()
endif()

#--------------------------------------------------------------------------------------------------

### check for stackgrowth
if (NOT CMAKE_CROSSCOMPILING)
  try_run(RUN_makecontext_VAR COMPILE_makecontext_VAR
    ${CMAKE_BINARY_DIR}
    ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_stackgrowth.c
    )
  file(READ "${CMAKE_BINARY_DIR}/conftestval" stack)
  if(stack MATCHES "down")
    set(PTH_STACKGROWTH "-1")
  endif()
  if(stack MATCHES "up")
    set(PTH_STACKGROWTH "1")
  endif()

endif()
###############
## System checks
##

#SG_CONFIGURE_PART([System checks...])
#AC_PROG_CC(xlC gcc cc) -auto
#AM_SANITY_CHECK -auto

#AC_PROG_MAKE_SET

#AC_PRINTF_NULL
try_run(RUN_PRINTF_NULL_VAR COMPILE_PRINTF_NULL_VAR
  ${CMAKE_BINARY_DIR}
  ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_printf_null.c
  )

if(RUN_PRINTF_NULL_VAR MATCHES "FAILED_TO_RUN")
  SET(PRINTF_NULL_WORKING "0")
else()
  SET(PRINTF_NULL_WORKING "1")
endif()

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
  write_file("${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_va_copy.c" "#include <stdlib.h>
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
  try_compile(COMPILE_VA_NULL_VAR
    ${CMAKE_BINARY_DIR}
    ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_va_copy.c
    )
  if(COMPILE_VA_NULL_VAR)
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
### check for getline
try_compile(COMPILE_RESULT_VAR
  ${CMAKE_BINARY_DIR}
  ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_getline.c
  )

if(NOT COMPILE_RESULT_VAR)
  SET(need_getline "#define SIMGRID_NEED_GETLINE 1")
  SET(SIMGRID_NEED_GETLINE 1)
else()
  SET(need_getline "")
  SET(SIMGRID_NEED_GETLINE 0)
endif()

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
      ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_snprintf.c
      )
  endif()

  if(CMAKE_CROSSCOMPILING)
    set(RUN_VSNPRINTF_FUNC "cross")
    set(PREFER_PORTABLE_VSNPRINTF 1)
  else()
    try_run(RUN_VSNPRINTF_FUNC_VAR COMPILE_VSNPRINTF_FUNC_VAR
      ${CMAKE_BINARY_DIR}
      ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_vsnprintf.c
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

### Check if OSX can compile with ucontext (with gcc 4.[1-5] it is broken)
if(APPLE)
  if(APPLE_NEED_GCC_VERSION GREATER COMPILER_C_VERSION_MAJOR_MINOR)
    message(STATUS "Ucontext can't be used with this version of gcc (must be greater than 4.5)")
    set(HAVE_UCONTEXT_H 0)
  endif()
endif()

### File to create

configure_file("${CMAKE_HOME_DIRECTORY}/src/context_sysv_config.h.in" 			"${CMAKE_BINARY_DIR}/src/context_sysv_config.h" @ONLY IMMEDIATE)

SET( CMAKEDEFINE "#cmakedefine" )
configure_file("${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/src/internal_config.h.in" 	"${CMAKE_BINARY_DIR}/src/internal_config.h" @ONLY IMMEDIATE)
configure_file("${CMAKE_BINARY_DIR}/src/internal_config.h" 			"${CMAKE_BINARY_DIR}/src/internal_config.h" @ONLY IMMEDIATE)
configure_file("${CMAKE_HOME_DIRECTORY}/include/simgrid_config.h.in" 		"${CMAKE_BINARY_DIR}/include/simgrid_config.h" @ONLY IMMEDIATE)

set(top_srcdir "${CMAKE_HOME_DIRECTORY}")
set(srcdir "${CMAKE_HOME_DIRECTORY}/src")

### Script used when simgrid is installed
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(includeflag "-I${CMAKE_INSTALL_PREFIX}/include -I${CMAKE_INSTALL_PREFIX}/include/smpi")
set(includedir "${CMAKE_INSTALL_PREFIX}/include")
set(libdir ${exec_prefix}/lib)
set(CMAKE_SMPI_COMMAND "export LD_LIBRARY_PATH=${CMAKE_INSTALL_PREFIX}/lib:${GTNETS_LIB_PATH}:${HAVE_NS3_LIB}:$LD_LIBRARY_PATH")

configure_file(${CMAKE_HOME_DIRECTORY}/include/smpi/smpif.h.in ${CMAKE_BINARY_DIR}/include/smpi/smpif.h @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpicc.in ${CMAKE_BINARY_DIR}/bin/smpicc @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpif2c.in ${CMAKE_BINARY_DIR}/bin/smpif2c @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpiff.in ${CMAKE_BINARY_DIR}/bin/smpiff @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpif90.in ${CMAKE_BINARY_DIR}/bin/smpif90 @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpirun.in ${CMAKE_BINARY_DIR}/bin/smpirun @ONLY)

### Script used when simgrid is compiling
set(includeflag "-I${CMAKE_HOME_DIRECTORY}/include -I${CMAKE_HOME_DIRECTORY}/include/smpi")
set(includeflag "${includeflag} -I${CMAKE_BINARY_DIR}/include -I${CMAKE_BINARY_DIR}/include/smpi")
set(includedir "${CMAKE_HOME_DIRECTORY}/include")
set(exec_prefix "${CMAKE_BINARY_DIR}/smpi_script/")
set(CMAKE_SMPI_COMMAND "export LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib:${GTNETS_LIB_PATH}:${HAVE_NS3_LIB}:$LD_LIBRARY_PATH")
set(libdir "${CMAKE_BINARY_DIR}/lib")

configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpicc.in ${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpif2c.in ${CMAKE_BINARY_DIR}/smpi_script/bin/smpif2c @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpiff.in ${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpif90.in ${CMAKE_BINARY_DIR}/smpi_script/bin/smpif90 @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpirun.in ${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun @ONLY)

set(top_builddir ${CMAKE_HOME_DIRECTORY})

if(NOT WIN32)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpicc)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpif2c)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpiff)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpif90)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpirun)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/smpi_script/bin/smpicc)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/smpi_script/bin/smpif2c)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/smpi_script/bin/smpiff)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/smpi_script/bin/smpif90)
  execute_process(COMMAND chmod a=rwx ${CMAKE_BINARY_DIR}/smpi_script/bin/smpirun)
endif()

set(generated_headers_to_install
  ${CMAKE_CURRENT_BINARY_DIR}/include/smpi/smpif.h
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
  ${CMAKE_BINARY_DIR}/bin/smpif2c
  ${CMAKE_BINARY_DIR}/bin/smpiff
  ${CMAKE_BINARY_DIR}/bin/smpif90
  ${CMAKE_BINARY_DIR}/bin/smpirun
  ${CMAKE_BINARY_DIR}/bin/colorize
  ${CMAKE_BINARY_DIR}/bin/simgrid_update_xml
  ${CMAKE_BINARY_DIR}/examples/smpi/tracing/smpi_traced.trace
  ${CMAKE_BINARY_DIR}/src/supernovae_sg.c
  ${CMAKE_BINARY_DIR}/src/supernovae_smpi.c
  )

if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_HOME_DIRECTORY}")
else()
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/hostfile ${CMAKE_BINARY_DIR}/examples/smpi/hostfile COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/msg/small_platform.xml ${CMAKE_BINARY_DIR}/examples/msg/small_platform.xml COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/msg/small_platform_with_routers.xml ${CMAKE_BINARY_DIR}/examples/msg/small_platform_with_routers.xml COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/msg/tracing/platform.xml ${CMAKE_BINARY_DIR}/examples/msg/tracing/platform.xml COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/actions0.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions0.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/actions1.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions1.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/actions_allReduce.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_allReduce.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/actions_barrier.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_barrier.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/actions_bcast.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_bcast.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/replay/actions_with_isend.txt ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_with_isend.txt COPYONLY)
  configure_file(${CMAKE_HOME_DIRECTORY}/teshsuite/smpi/hostfile ${CMAKE_BINARY_DIR}/teshsuite/smpi/hostfile COPYONLY)

  set(generated_files_to_clean
    ${generated_files_to_clean}
    ${CMAKE_BINARY_DIR}/examples/smpi/hostfile
    ${CMAKE_BINARY_DIR}/examples/msg/small_platform.xml
    ${CMAKE_BINARY_DIR}/examples/msg/small_platform_with_routers.xml
    ${CMAKE_BINARY_DIR}/examples/msg/tracing/platform.xml
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions0.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions1.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_allReduce.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_barrier.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_bcast.txt
    ${CMAKE_BINARY_DIR}/examples/smpi/replay/actions_with_isend.txt
    ${CMAKE_BINARY_DIR}/teshsuite/smpi/hostfile
    )
endif()

SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
  "${generated_files_to_clean}")

IF(${ARCH_32_BITS})
  set(WIN_ARCH "32")
ELSE()
  set(WIN_ARCH "64")
ENDIF()
configure_file("${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/src/simgrid.nsi.in" 	"${CMAKE_BINARY_DIR}/simgrid.nsi" @ONLY IMMEDIATE)
