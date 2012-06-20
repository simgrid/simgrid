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
    ELSE(${ARCH_32_BITS})
        message(STATUS "System processor: amd64")
        set(SIMGRID_SYSTEM_PROCESSOR "amd64")
        set(PROCESSOR_x86_64 1)
        set(PROCESSOR_i686 0)
    ENDIF(${ARCH_32_BITS})          
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
    
ELSE(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64") #PROCESSOR NOT fIND
    message(STATUS "PROCESSOR NOT FOUND: ${CMAKE_SYSTEM_PROCESSOR}")
    
ENDIF(CMAKE_SYSTEM_PROCESSOR MATCHES ".86")

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
else(WIN32)
include(FindPCRE)  
endif(WIN32)

set(HAVE_GTNETS 0)
if(enable_gtnets)	
	include(FindGTnets)
endif(enable_gtnets)
if(enable_smpi)
	include(FindF2c)
	if(HAVE_F2C_H)
        SET(HAVE_SMPI 1)
    endif(HAVE_F2C_H)
endif(enable_smpi)
if(enable_lua)
	include(FindLua51Simgrid)
endif(enable_lua)
set(HAVE_NS3 0)
if(enable_ns3)
	include(FindNS3)
endif(enable_ns3)

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
else(HAVE_thread_storage_run)
	set(HAVE_THREAD_LOCAL_STORAGE 1)
endif(HAVE_thread_storage_run)

# Our usage of mmap is Linux-specific (flag MAP_ANONYMOUS), but kFreeBSD uses a GNU libc
IF(NOT "${CMAKE_SYSTEM}" MATCHES "Linux" AND NOT "${CMAKE_SYSTEM}" MATCHES "kFreeBSD" AND NOT "${CMAKE_SYSTEM}" MATCHES "GNU")
  SET(HAVE_MMAP 0)
  message(STATUS "Warning: MMAP is thought as non functional on this architecture (${CMAKE_SYSTEM})")
ENDIF(NOT "${CMAKE_SYSTEM}" MATCHES "Linux" AND NOT "${CMAKE_SYSTEM}" MATCHES "kFreeBSD" AND NOT "${CMAKE_SYSTEM}" MATCHES "GNU")

if(WIN32) #THOSE FILES ARE FUNCTIONS ARE NOT DETECTED BUT THEY SHOULD...
    set(HAVE_UCONTEXT_H 1)
    set(HAVE_MAKECONTEXT 1)
    set(HAVE_SNPRINTF 1)
    set(HAVE_VSNPRINTF 1)
endif(WIN32)

set(CONTEXT_UCONTEXT 0)
SET(CONTEXT_THREADS 0)
SET(HAVE_TRACING 1)

if(enable_tracing)
	SET(HAVE_TRACING 1)
else(enable_tracing)	
	SET(HAVE_TRACING 0)
endif(enable_tracing)

if(enable_jedule)
	SET(HAVE_JEDULE 1)
endif(enable_jedule)

if(enable_latency_bound_tracking)
	SET(HAVE_LATENCY_BOUND_TRACKING 1)
else(enable_latency_bound_tracking)
  if(enable_gtnets)
    message(STATUS "Warning : Turning latency_bound_tracking to ON because GTNeTs is ON")
    SET(enable_latency_bound_tracking ON)
    SET(HAVE_LATENCY_BOUND_TRACKING 1)
  else(enable_gtnets)
    SET(HAVE_LATENCY_BOUND_TRACKING 0)
  endif(enable_gtnets)
endif(enable_latency_bound_tracking)

if(enable_model-checking AND HAVE_MMAP)
	SET(HAVE_MC 1)
	SET(MMALLOC_WANT_OVERRIDE_LEGACY 1)
else(enable_model-checking AND HAVE_MMAP)
	SET(HAVE_MC 0)
	SET(MMALLOC_WANT_OVERRIDE_LEGACY 0)
endif(enable_model-checking AND HAVE_MMAP)

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
endif(pthread)

if(pthread)
	### Test that we have a way to create semaphores

    if(HAVE_SEM_OPEN_LIB)
    
        exec_program(
        "${CMAKE_C_COMPILER} ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_sem_open.c -lpthread -o testprog"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        OUTPUT_VARIABLE HAVE_SEM_OPEN_compil
        )
    
      	if(HAVE_SEM_OPEN_compil)
    		set(HAVE_SEM_OPEN 0)
    		message(STATUS "Warning: sem_open not compilable")
    		message(STATUS "HAVE_SEM_OPEN_comp_output: ${HAVE_SEM_OPEN_comp_output}")    	
    	else(HAVE_SEM_OPEN_compil)
    		set(HAVE_SEM_OPEN 1)
            message(STATUS "sem_open is compilable")
    	endif(HAVE_SEM_OPEN_compil)
        
        exec_program("${CMAKE_BINARY_DIR}/testprog" RETURN_VALUE HAVE_SEM_OPEN_run OUTPUT_VARIABLE var_compil)
        file(REMOVE "${CMAKE_BINARY_DIR}/testprog*")
    	
    	if(NOT HAVE_SEM_OPEN_run)
    	    set(HAVE_SEM_OPEN 1)
            message(STATUS "sem_open is executable")
        else(NOT HAVE_SEM_OPEN_run)
    		set(HAVE_SEM_OPEN 0)
    	    message(STATUS "Warning: sem_open not executable")    	
        endif(NOT HAVE_SEM_OPEN_run)	
    
    else(HAVE_SEM_OPEN_LIB)
		set(HAVE_SEM_OPEN 0)
  	endif(HAVE_SEM_OPEN_LIB)

  	if(HAVE_SEM_INIT_LIB)
        exec_program(
        "${CMAKE_C_COMPILER} ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_sem_init.c -lpthread -o testprog"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        OUTPUT_VARIABLE HAVE_SEM_INIT_compil
        )
    
      	if(HAVE_SEM_INIT_compil)
    		set(HAVE_SEM_INIT 0)
    		message(STATUS "Warning: sem_init not compilable")
    		message(STATUS "HAVE_SEM_INIT_comp_output: ${HAVE_SEM_OPEN_comp_output}")    	
    	else(HAVE_SEM_INIT_compil)
    		set(HAVE_SEM_INIT 1)
            message(STATUS "sem_init is compilable")
    	endif(HAVE_SEM_INIT_compil)

        exec_program("${CMAKE_BINARY_DIR}/testprog" RETURN_VALUE HAVE_SEM_INIT_run OUTPUT_VARIABLE var_compil)
        file(REMOVE "${CMAKE_BINARY_DIR}/testprog*")
        
    	if(NOT HAVE_SEM_INIT_run)
    	    set(HAVE_SEM_INIT 1)
            message(STATUS "sem_init is executable")
        else(NOT HAVE_SEM_INIT_run)
    		set(HAVE_SEM_INIT 0)
    	    message(STATUS "Warning: sem_init not executable")
        endif(NOT HAVE_SEM_INIT_run)	
  	endif(HAVE_SEM_INIT_LIB)

	if(NOT HAVE_SEM_OPEN AND NOT HAVE_SEM_INIT)
		message(FATAL_ERROR "Semaphores are not usable (neither sem_open nor sem_init is both compilable and executable), but they are mandatory to threads (you may need to mount /dev).")
	endif(NOT HAVE_SEM_OPEN AND NOT HAVE_SEM_INIT)

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
		else(HAVE_SEM_TIMEDWAIT_run)
			set(HAVE_SEM_TIMEDWAIT 1)
            message(STATUS "timedwait is compilable")
		endif(HAVE_SEM_TIMEDWAIT_run)
	endif(HAVE_SEM_TIMEDWAIT_LIB)

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
		else(HAVE_MUTEX_TIMEDLOCK_run)
			message(STATUS "timedlock is compilable")
			set(HAVE_MUTEX_TIMEDLOCK 1)
		endif(HAVE_MUTEX_TIMEDLOCK_run)
	endif(HAVE_MUTEX_TIMEDLOCK_LIB)
endif(pthread)

# AC_CHECK_MCSC(mcsc=yes, mcsc=no) 
set(mcsc_flags "")
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	set(mcsc_flags "-D_XOPEN_SOURCE")
endif(CMAKE_SYSTEM_NAME MATCHES "Darwin")

if(WIN32)
    if(ARCH_32_BITS)
    set(mcsc_flags "-D_XBT_WIN32 -D_I_X86_ -I${CMAKE_HOME_DIRECTORY}/include/xbt -I${CMAKE_HOME_DIRECTORY}/src/xbt")
    else(ARCH_32_BITS)
    set(mcsc_flags "-D_XBT_WIN32 -D_AMD64_ -I${CMAKE_HOME_DIRECTORY}/include/xbt -I${CMAKE_HOME_DIRECTORY}/src/xbt")
    endif(ARCH_32_BITS)
endif(WIN32)

IF(CMAKE_CROSSCOMPILING)
	IF(WIN32)
		set(windows_context "yes")
		set(IS_WINDOWS 1)	
	ENDIF(WIN32)
ELSE(CMAKE_CROSSCOMPILING)
    file(REMOVE "${CMAKE_BINARY_DIR}/testprog*")
    file(REMOVE ${CMAKE_BINARY_DIR}/conftestval)
    exec_program(
                 "${CMAKE_C_COMPILER} ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_AC_CHECK_MCSC.c ${mcsc_flags} -o testprog"
                 WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/
                 OUTPUT_VARIABLE COMPILE_mcsc_VAR)

    if(NOT COMPILE_mcsc_VAR)
        message(STATUS "prog_AC_CHECK_MCSC.c is compilable")
        exec_program("${CMAKE_BINARY_DIR}/testprog" OUTPUT_VARIABLE var_compil)
    else(NOT COMPILE_mcsc_VAR)
        message(STATUS "prog_AC_CHECK_MCSC.c is not compilable")
    endif(NOT COMPILE_mcsc_VAR)
    file(REMOVE "${CMAKE_BINARY_DIR}/testprog*")
    
	if(EXISTS "${CMAKE_BINARY_DIR}/conftestval")
		file(READ "${CMAKE_BINARY_DIR}/conftestval" mcsc)
		STRING(REPLACE "\n" "" mcsc "${mcsc}")
		if(mcsc)
			set(mcsc "yes")
			set(HAVE_UCONTEXT_H 1)
		else(mcsc)
			set(mcsc "no")
		endif(mcsc)
    else(EXISTS "${CMAKE_BINARY_DIR}/conftestval")
		set(mcsc "no")
	endif(EXISTS "${CMAKE_BINARY_DIR}/conftestval")
	
	message(STATUS "mcsc: ${mcsc}")   
ENDIF(CMAKE_CROSSCOMPILING)

if(mcsc MATCHES "no" AND pthread)
	if(HAVE_WINDOWS_H)
		set(windows_context "yes")
		set(IS_WINDOWS 1)
	elseif(HAVE_WINDOWS_H)
		message(FATAL_ERROR "no appropriate backend found")
	endif(HAVE_WINDOWS_H)
endif(mcsc MATCHES "no" AND pthread)

#Only windows

if(WIN32)
	if(NOT HAVE_WINDOWS_H)
		message(FATAL_ERROR "no appropriate backend found windows")
	endif(NOT HAVE_WINDOWS_H)
endif(WIN32)

if(windows_context MATCHES "yes")
	message(STATUS "Context change to windows")
endif(windows_context MATCHES "yes")

#If can have both context

if(mcsc)
	set(CONTEXT_UCONTEXT 1)
endif(mcsc)

if(pthread)
	set(CONTEXT_THREADS 1)
endif(pthread)


###############
## GIT version check
##
if(EXISTS ${CMAKE_HOME_DIRECTORY}/.git/ AND NOT WIN32)
exec_program("git remote | head -n 1" OUTPUT_VARIABLE remote RETURN_VALUE ret)
exec_program("git config --get remote.${remote}.url" OUTPUT_VARIABLE url RETURN_VALUE ret)

if(url)
	exec_program("git --git-dir=${CMAKE_HOME_DIRECTORY}/.git log --oneline -1" OUTPUT_VARIABLE "GIT_VERSION")
	message(STATUS "Git version: ${GIT_VERSION}")
	exec_program("git --git-dir=${CMAKE_HOME_DIRECTORY}/.git log -n 1 --format=%ai ." OUTPUT_VARIABLE "GIT_DATE")
	message(STATUS "Git date: ${GIT_DATE}")
	string(REGEX REPLACE " .*" "" GIT_VERSION "${GIT_VERSION}")
	STRING(REPLACE " +0000" "" GIT_DATE ${GIT_DATE})
	STRING(REPLACE " " "~" GIT_DATE ${GIT_DATE})
	STRING(REPLACE ":" "-" GIT_DATE ${GIT_DATE})
endif(url)
endif(EXISTS ${CMAKE_HOME_DIRECTORY}/.git/ AND NOT WIN32)

###################################
## SimGrid and GRAS specific checks
##

IF(NOT CMAKE_CROSSCOMPILING)
# Check architecture signature begin
try_run(RUN_GRAS_VAR COMPILE_GRAS_VAR
	${CMAKE_BINARY_DIR}
	${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_GRAS_ARCH.c
	RUN_OUTPUT_VARIABLE var1
	)
if(BIGENDIAN)
  set(val_big "B${var1}")
  set(GRAS_BIGENDIAN 1)
else(BIGENDIAN)
  set(val_big "l${var1}")
  set(GRAS_BIGENDIAN 0)
endif(BIGENDIAN)

# The syntax of this magic string is given in src/xbt/datadesc/ddt_convert.c
# It kinda matches the values that the xbt_arch_desc_t structure can take

# Basically, the syntax is one char l or B for endianness (little or Big)
#   then there is a bunch of blocks separated by _.  
# C block is for char, I block for integers, P block for pointers and
#   D block for floating points
# For each block there is an amount of chuncks separated by :, each of
#   them describing a data size. For example there is only one chunk
#   in the char block, because no architecture provide several sizes
#   of chars. In integer block, there is 4 chunks: "short int", "int",
#   "long int", "long long int". There is 2 pointer chunks for data
#   pointers and pointers on functions (thanks to the AMD64 madness).
#   Thee two floating points chuncks are for "float" and "double".
# Each chunk is of the form datasize/minimal_alignment_size

# These informations are used to convert a data stream from one
#    formalism to another. Only the GRAS_ARCH is transfered in the
#    stream, and it it of cruxial importance to keep these detection
#    information here synchronized with the data hardcoded in the
#    source in src/xbt/datadesc/ddt_convert.c 

# If you add something here (like a previously unknown architecture),
#    please add it to the source code too. 
# Please do not modify stuff here since it'd break the GRAS protocol.
#     If you really need to change stuff, please also bump
#    GRAS_PROTOCOL_VERSION in src/gras/Msg/msg_interface.h

SET(GRAS_THISARCH "none")

if(val_big MATCHES "l_C:1/1:_I:2/1:4/1:4/1:8/1:_P:4/1:4/1:_D:4/1:8/1:")
	#gras_arch=0; gras_size=32; gras_arch_name=little32_1;
	SET(GRAS_ARCH_32_BITS 1)
	SET(GRAS_THISARCH 0)
endif(val_big MATCHES "l_C:1/1:_I:2/1:4/1:4/1:8/1:_P:4/1:4/1:_D:4/1:8/1:")
if(val_big MATCHES "l_C:1/1:_I:2/2:4/2:4/2:8/2:_P:4/2:4/2:_D:4/2:8/2:")
	#gras_arch=1; gras_size=32; gras_arch_name=little32_2;
	SET(GRAS_ARCH_32_BITS 1)
	SET(GRAS_THISARCH 1)
endif(val_big MATCHES "l_C:1/1:_I:2/2:4/2:4/2:8/2:_P:4/2:4/2:_D:4/2:8/2:")
if(val_big MATCHES "l_C:1/1:_I:2/2:4/4:4/4:8/4:_P:4/4:4/4:_D:4/4:8/4:") 
	#gras_arch=2; gras_size=32; gras_arch_name=little32_4;
	SET(GRAS_ARCH_32_BITS 1)
	SET(GRAS_THISARCH 2)
endif(val_big MATCHES "l_C:1/1:_I:2/2:4/4:4/4:8/4:_P:4/4:4/4:_D:4/4:8/4:")
if(val_big MATCHES "l_C:1/1:_I:2/2:4/4:4/4:8/8:_P:4/4:4/4:_D:4/4:8/8:") 
	#gras_arch=3; gras_size=32; gras_arch_name=little32_8;
	SET(GRAS_ARCH_32_BITS 1)
	SET(GRAS_THISARCH 3)
endif(val_big MATCHES "l_C:1/1:_I:2/2:4/4:4/4:8/8:_P:4/4:4/4:_D:4/4:8/8:") 
if(val_big MATCHES "l_C:1/1:_I:2/2:4/4:8/8:8/8:_P:8/8:8/8:_D:4/4:8/8:") 
	#gras_arch=4; gras_size=64; gras_arch_name=little64;
	SET(GRAS_ARCH_32_BITS 0)
	SET(GRAS_THISARCH 4)
endif(val_big MATCHES "l_C:1/1:_I:2/2:4/4:8/8:8/8:_P:8/8:8/8:_D:4/4:8/8:")
if(val_big MATCHES "l_C:1/1:_I:2/2:4/4:4/4:8/8:_P:8/8:8/8:_D:4/4:8/8:") 
	#gras_arch=5; gras_size=64; gras_arch_name=little64_2;
	SET(GRAS_ARCH_32_BITS 0)
	SET(GRAS_THISARCH 5)
endif(val_big MATCHES "l_C:1/1:_I:2/2:4/4:4/4:8/8:_P:8/8:8/8:_D:4/4:8/8:")

if(val_big MATCHES "B_C:1/1:_I:2/2:4/4:4/4:8/8:_P:4/4:4/4:_D:4/4:8/8:") 
	#gras_arch=6; gras_size=32; gras_arch_name=big32_8;
	SET(GRAS_ARCH_32_BITS 1)
	SET(GRAS_THISARCH 6)
endif(val_big MATCHES "B_C:1/1:_I:2/2:4/4:4/4:8/8:_P:4/4:4/4:_D:4/4:8/8:")
if(val_big MATCHES "B_C:1/1:_I:2/2:4/4:4/4:8/8:_P:4/4:4/4:_D:4/4:8/4:") 
	#gras_arch=7; gras_size=32; gras_arch_name=big32_8_4;
	SET(GRAS_ARCH_32_BITS 1)
	SET(GRAS_THISARCH 7)
endif(val_big MATCHES "B_C:1/1:_I:2/2:4/4:4/4:8/8:_P:4/4:4/4:_D:4/4:8/4:")
if(val_big MATCHES "B_C:1/1:_I:2/2:4/4:4/4:8/4:_P:4/4:4/4:_D:4/4:8/4:") 
	#gras_arch=8; gras_size=32; gras_arch_name=big32_4;
	SET(GRAS_ARCH_32_BITS 1)
	SET(GRAS_THISARCH 8)
endif(val_big MATCHES "B_C:1/1:_I:2/2:4/4:4/4:8/4:_P:4/4:4/4:_D:4/4:8/4:")
if(val_big MATCHES "B_C:1/1:_I:2/2:4/2:4/2:8/2:_P:4/2:4/2:_D:4/2:8/2:") 
	#gras_arch=9; gras_size=32; gras_arch_name=big32_2;
	SET(GRAS_ARCH_32_BITS 1)
	SET(GRAS_THISARCH 9)
endif(val_big MATCHES "B_C:1/1:_I:2/2:4/2:4/2:8/2:_P:4/2:4/2:_D:4/2:8/2:") 
if(val_big MATCHES "B_C:1/1:_I:2/2:4/4:8/8:8/8:_P:8/8:8/8:_D:4/4:8/8:") 
	#gras_arch=10; gras_size=64; gras_arch_name=big64;
	SET(GRAS_ARCH_32_BITS 0)
	SET(GRAS_THISARCH 10)
endif(val_big MATCHES "B_C:1/1:_I:2/2:4/4:8/8:8/8:_P:8/8:8/8:_D:4/4:8/8:")
if(val_big MATCHES "B_C:1/1:_I:2/2:4/4:8/8:8/8:_P:8/8:8/8:_D:4/4:8/4:") 
	#gras_arch=11; gras_size=64; gras_arch_name=big64_8_4;
	SET(GRAS_ARCH_32_BITS 0)
	SET(GRAS_THISARCH 11)
endif(val_big MATCHES "B_C:1/1:_I:2/2:4/4:8/8:8/8:_P:8/8:8/8:_D:4/4:8/4:") 

if(GRAS_THISARCH MATCHES "none")
    message(STATUS "architecture: ${val_big}")
    message(FATAL_ERROR "GRAS_THISARCH is empty: '${GRAS_THISARCH}'")  
endif(GRAS_THISARCH MATCHES "none")

# Check architecture signature end
try_run(RUN_GRAS_VAR COMPILE_GRAS_VAR
	${CMAKE_BINARY_DIR}
	${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_GRAS_CHECK_STRUCT_COMPACTION.c
	RUN_OUTPUT_VARIABLE var2
	)
separate_arguments(var2)
foreach(var_tmp ${var2})
	set(${var_tmp} 1)
endforeach(var_tmp ${var2})

# Check for [SIZEOF_MAX]
try_run(RUN_SM_VAR COMPILE_SM_VAR
	${CMAKE_BINARY_DIR}
	${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_max_size.c
	RUN_OUTPUT_VARIABLE var3
	)
message(STATUS "SIZEOF_MAX ${var3}")
SET(SIZEOF_MAX ${var3})
ENDIF(NOT CMAKE_CROSSCOMPILING)

#--------------------------------------------------------------------------------------------------

set(makecontext_CPPFLAGS_2 "")
if(HAVE_MAKECONTEXT OR WIN32)
	set(makecontext_CPPFLAGS "-DTEST_makecontext")
	if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
		set(makecontext_CPPFLAGS_2 "-D_XOPEN_SOURCE")
	endif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	
    if(WIN32)
        if(ARCH_32_BITS)
	    set(makecontext_CPPFLAGS "-DTEST_makecontext -D_I_X86_")
	    else(ARCH_32_BITS)
	    set(makecontext_CPPFLAGS "-DTEST_makecontext -D_AMD64_")
	    endif(ARCH_32_BITS)
	    set(makecontext_CPPFLAGS_2 "-D_XBT_WIN32 -I${CMAKE_HOME_DIRECTORY}/include/xbt -I${CMAKE_HOME_DIRECTORY}/src/xbt")
	endif(WIN32)

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
	else(EXISTS ${CMAKE_BINARY_DIR}/conftestval)
#	    message(FATAL_ERROR "makecontext is not compilable")
	endif(EXISTS ${CMAKE_BINARY_DIR}/conftestval)
endif(HAVE_MAKECONTEXT OR WIN32)

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
endif(stack MATCHES "down")
if(stack MATCHES "up")
	set(PTH_STACKGROWTH "1")
endif(stack MATCHES "up")

endif(NOT CMAKE_CROSSCOMPILING)
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
else(RUN_PRINTF_NULL_VAR MATCHES "FAILED_TO_RUN")
SET(PRINTF_NULL_WORKING "1")
endif(RUN_PRINTF_NULL_VAR MATCHES "FAILED_TO_RUN")

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
		endif(${fctbis} STREQUAL "va_copy((d),(s))")

		if(${fctbis} STREQUAL "VA_COPY((d),(s))")
			set(ac_cv_va_copy "GCM")
			set(__VA_COPY_USE_GCM "VA_COPY((d),(s))")
		endif(${fctbis} STREQUAL "VA_COPY((d),(s))")

		if(${fctbis} STREQUAL "__va_copy((d),(s))")
			set(ac_cv_va_copy "GCH")
			set(__VA_COPY_USE_GCH "__va_copy((d),(s))")
		endif(${fctbis} STREQUAL "__va_copy((d),(s))")

		if(${fctbis} STREQUAL "__builtin_va_copy((d),(s))")
			set(ac_cv_va_copy "GCB")
			set(__VA_COPY_USE_GCB "__builtin_va_copy((d),(s))")
		endif(${fctbis} STREQUAL "__builtin_va_copy((d),(s))")

		if(${fctbis} STREQUAL "do { (d) = (s) } while (0)")
			set(ac_cv_va_copy "ASS")
			set(__VA_COPY_USE_ASS "do { (d) = (s); } while (0)")
		endif(${fctbis} STREQUAL "do { (d) = (s) } while (0)")

		if(${fctbis} STREQUAL "do { *(d) = *(s) } while (0)")
			set(ac_cv_va_copy "ASP")
			set(__VA_COPY_USE_ASP "do { *(d) = *(s); } while (0)")
		endif(${fctbis} STREQUAL "do { *(d) = *(s) } while (0)")

		if(${fctbis} STREQUAL "memcpy((void *)&(d), (void *)&(s), sizeof(s))")
			set(ac_cv_va_copy "CPS")
			set(__VA_COPY_USE_CPS "memcpy((void *)&(d), (void *)&(s), sizeof(s))")
		endif(${fctbis} STREQUAL "memcpy((void *)&(d), (void *)&(s), sizeof(s))")

		if(${fctbis} STREQUAL "memcpy((void *)(d), (void *)(s), sizeof(*(s)))")
			set(ac_cv_va_copy "CPP")
			set(__VA_COPY_USE_CPP "memcpy((void *)(d), (void *)(s), sizeof(*(s)))")
		endif(${fctbis} STREQUAL "memcpy((void *)(d), (void *)(s), sizeof(*(s)))")
				
		if(NOT STATUS_OK)
		set(__VA_COPY_USE "__VA_COPY_USE_${ac_cv_va_copy}(d, s)")
		endif(NOT STATUS_OK)
		set(STATUS_OK "1")
		
	endif(COMPILE_VA_NULL_VAR)
	
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
else(NOT COMPILE_RESULT_VAR)
SET(need_getline "")
SET(SIMGRID_NEED_GETLINE 0)
endif(NOT COMPILE_RESULT_VAR)

### check for a working snprintf
if(HAVE_SNPRINTF AND HAVE_VSNPRINTF OR WIN32)
    if(WIN32)
        #set(HAVE_SNPRINTF 1)
        #set(HAVE_VSNPRINTF 1)
    endif(WIN32)
    
	if(CMAKE_CROSSCOMPILING)
		set(RUN_SNPRINTF_FUNC "cross")
		#set(PREFER_PORTABLE_SNPRINTF 1)
	else(CMAKE_CROSSCOMPILING)
  	    try_run(RUN_SNPRINTF_FUNC_VAR COMPILE_SNPRINTF_FUNC_VAR
	  	${CMAKE_BINARY_DIR}
		${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_snprintf.c
	    )	
	endif(CMAKE_CROSSCOMPILING)

	if(CMAKE_CROSSCOMPILING)
		set(RUN_VSNPRINTF_FUNC "cross")
		set(PREFER_PORTABLE_VSNPRINTF 1)
	else(CMAKE_CROSSCOMPILING)
  	   try_run(RUN_VSNPRINTF_FUNC_VAR COMPILE_VSNPRINTF_FUNC_VAR
		${CMAKE_BINARY_DIR}
		${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/test_prog/prog_vsnprintf.c
	   )
	endif(CMAKE_CROSSCOMPILING)
	
	set(PREFER_PORTABLE_SNPRINTF 0)
	if(RUN_VSNPRINTF_FUNC_VAR MATCHES "FAILED_TO_RUN")
		set(PREFER_PORTABLE_SNPRINTF 1)
	endif(RUN_VSNPRINTF_FUNC_VAR MATCHES "FAILED_TO_RUN")
	if(RUN_SNPRINTF_FUNC_VAR MATCHES "FAILED_TO_RUN")
		set(PREFER_PORTABLE_SNPRINTF 1)
	endif(RUN_SNPRINTF_FUNC_VAR MATCHES "FAILED_TO_RUN")
endif(HAVE_SNPRINTF AND HAVE_VSNPRINTF OR WIN32)

### check for asprintf function familly
if(HAVE_ASPRINTF)
	SET(simgrid_need_asprintf "")
	SET(NEED_ASPRINTF 0)
else(HAVE_ASPRINTF)
	SET(simgrid_need_asprintf "#define SIMGRID_NEED_ASPRINTF 1")
	SET(NEED_ASPRINTF 1)
endif(HAVE_ASPRINTF)

if(HAVE_VASPRINTF)
	SET(simgrid_need_vasprintf "")
	SET(NEED_VASPRINTF 0)
else(HAVE_VASPRINTF)
	SET(simgrid_need_vasprintf "#define SIMGRID_NEED_VASPRINTF 1")
	SET(NEED_VASPRINTF 1)
endif(HAVE_VASPRINTF)

### check for addr2line

find_path(ADDR2LINE NAMES addr2line	PATHS NO_DEFAULT_PATHS	)
if(ADDR2LINE)
set(ADDR2LINE "${ADDR2LINE}/addr2line")
endif(ADDR2LINE)



### Check if OSX can compile with ucontext (with gcc 4.[1-5] it is broken)
if(APPLE)
    if(APPLE_NEED_GCC_VERSION GREATER COMPILER_C_VERSION_MAJOR_MINOR)
        message(STATUS "Ucontext can't be used with this version of gcc (must be greater than 4.5)")
        set(HAVE_UCONTEXT_H 0)
    endif(APPLE_NEED_GCC_VERSION GREATER COMPILER_C_VERSION_MAJOR_MINOR)
endif(APPLE)

### File to create

configure_file("${CMAKE_HOME_DIRECTORY}/src/context_sysv_config.h.in" 			"${CMAKE_BINARY_DIR}/src/context_sysv_config.h" @ONLY IMMEDIATE)

SET( CMAKEDEFINE "#cmakedefine" )
configure_file("${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/src/gras_config.h.in" 	"${CMAKE_BINARY_DIR}/src/gras_config.h" @ONLY IMMEDIATE)
configure_file("${CMAKE_BINARY_DIR}/src/gras_config.h" 			"${CMAKE_BINARY_DIR}/src/gras_config.h" @ONLY IMMEDIATE)
configure_file("${CMAKE_HOME_DIRECTORY}/include/simgrid_config.h.in" 		"${CMAKE_BINARY_DIR}/include/simgrid_config.h" @ONLY IMMEDIATE)

set(top_srcdir "${CMAKE_HOME_DIRECTORY}")
set(srcdir "${CMAKE_HOME_DIRECTORY}/src")

set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(includedir ${CMAKE_INSTALL_PREFIX}/include)
set(top_builddir ${CMAKE_HOME_DIRECTORY})
set(libdir ${exec_prefix}/lib)
set(CMAKE_LINKARGS "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_SMPI_COMMAND "export LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib:${GTNETS_LIB_PATH}:${HAVE_NS3_LIB}:$LD_LIBRARY_PATH")

configure_file(${CMAKE_HOME_DIRECTORY}/include/smpi/smpif.h.in ${CMAKE_BINARY_DIR}/include/smpi/smpif.h @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpicc.in ${CMAKE_BINARY_DIR}/bin/smpicc @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpif2c.in ${CMAKE_BINARY_DIR}/bin/smpif2c @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpiff.in ${CMAKE_BINARY_DIR}/bin/smpiff @ONLY)
configure_file(${CMAKE_HOME_DIRECTORY}/src/smpi/smpirun.in ${CMAKE_BINARY_DIR}/bin/smpirun @ONLY)

if(NOT WIN32)
exec_program("chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpicc" OUTPUT_VARIABLE OKITOKI)
exec_program("chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpif2c" OUTPUT_VARIABLE OKITOKI)
exec_program("chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpiff" OUTPUT_VARIABLE OKITOKI)
exec_program("chmod a=rwx ${CMAKE_BINARY_DIR}/bin/smpirun" OUTPUT_VARIABLE OKITOKI)
endif(NOT WIN32)

set(generated_headers_to_install
	${CMAKE_CURRENT_BINARY_DIR}/include/smpi/smpif.h
	${CMAKE_CURRENT_BINARY_DIR}/include/simgrid_config.h
)

set(generated_headers
    ${CMAKE_CURRENT_BINARY_DIR}/src/context_sysv_config.h
    ${CMAKE_CURRENT_BINARY_DIR}/src/gras_config.h
)

set(generated_files_to_clean
${generated_headers}
${generated_headers_to_install}
${CMAKE_BINARY_DIR}/bin/smpicc
${CMAKE_BINARY_DIR}/bin/smpif2c
${CMAKE_BINARY_DIR}/bin/smpiff
${CMAKE_BINARY_DIR}/bin/smpirun
${CMAKE_BINARY_DIR}/bin/colorize
${CMAKE_BINARY_DIR}/bin/simgrid_update_xml
${CMAKE_BINARY_DIR}/examples/smpi/tracing/smpi_traced.trace
${CMAKE_BINARY_DIR}/src/supernovae_sg.c
${CMAKE_BINARY_DIR}/src/supernovae_gras.c
${CMAKE_BINARY_DIR}/src/supernovae_smpi.c
)

if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_HOME_DIRECTORY}")
else("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_HOME_DIRECTORY}")
	configure_file(${CMAKE_HOME_DIRECTORY}/examples/smpi/hostfile ${CMAKE_BINARY_DIR}/examples/smpi/hostfile COPYONLY)
	configure_file(${CMAKE_HOME_DIRECTORY}/examples/msg/small_platform.xml ${CMAKE_BINARY_DIR}/examples/msg/small_platform.xml COPYONLY)
	configure_file(${CMAKE_HOME_DIRECTORY}/examples/msg/small_platform_with_routers.xml ${CMAKE_BINARY_DIR}/examples/msg/small_platform_with_routers.xml COPYONLY)
	configure_file(${CMAKE_HOME_DIRECTORY}/examples/msg/tracing/platform.xml ${CMAKE_BINARY_DIR}/examples/msg/tracing/platform.xml COPYONLY)
	
	set(generated_files_to_clean
		${generated_files_to_clean}
		${CMAKE_BINARY_DIR}/examples/smpi/hostfile
		${CMAKE_BINARY_DIR}/examples/msg/small_platform.xml
		${CMAKE_BINARY_DIR}/examples/msg/small_platform_with_routers.xml
		${CMAKE_BINARY_DIR}/examples/msg/tracing/platform.xml
		)
endif("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_HOME_DIRECTORY}")

SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
"${generated_files_to_clean}")


IF(${ARCH_32_BITS})
  set(WIN_ARCH "32")
ELSE(${ARCH_32_BITS})
    set(WIN_ARCH "64")
ENDIF(${ARCH_32_BITS})
configure_file("${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/src/simgrid.nsi.in" 	"${CMAKE_BINARY_DIR}/simgrid.nsi" @ONLY IMMEDIATE)
