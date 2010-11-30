set(CMAKE_MODULE_PATH 
${CMAKE_MODULE_PATH}
${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/Modules
)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(TestBigEndian)
include(FindGraphviz)
TEST_BIG_ENDIAN(BIGENDIAN)

# Checks for header libraries functions.
CHECK_LIBRARY_EXISTS(pthread 	pthread_create 		NO_DEFAULT_PATHS pthread)
CHECK_LIBRARY_EXISTS(pthread 	sem_init 		NO_DEFAULT_PATHS HAVE_SEM_INIT_LIB)
CHECK_LIBRARY_EXISTS(pthread 	sem_timedwait 		NO_DEFAULT_PATHS HAVE_SEM_TIMEDWAIT_LIB)
CHECK_LIBRARY_EXISTS(pthread 	pthread_mutex_timedlock NO_DEFAULT_PATHS HAVE_MUTEX_TIMEDLOCK_LIB)
CHECK_LIBRARY_EXISTS(rt 	clock_gettime 		NO_DEFAULT_PATHS HAVE_POSIX_GETTIME)

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
CHECK_INCLUDE_FILE("errno.h" HAVE_ERRNO_H)
CHECK_INCLUDE_FILE("unistd.h" HAVE_UNISTD_H)
CHECK_INCLUDE_FILE("execinfo.h" HAVE_EXECINFO_H)
CHECK_INCLUDE_FILE("signal.h" HAVE_SIGNAL_H)
CHECK_INCLUDE_FILE("sys/time.h" HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILE("time.h" HAVE_TIME_H)
CHECK_INCLUDE_FILE("dlfcn.h" HAVE_DLFCN_H)
CHECK_INCLUDE_FILE("inttypes.h" HAVE_INTTYPES_H)
CHECK_INCLUDE_FILE("memory.h" HAVE_MEMORY_H)
CHECK_INCLUDE_FILE("stdlib.h" HAVE_STDLIB_H)
CHECK_INCLUDE_FILE("strings.h" HAVE_STRINGS_H)
CHECK_INCLUDE_FILE("string.h" HAVE_STRING_H)
CHECK_INCLUDE_FILE("ucontext.h" HAVE_UCONTEXT_H)
CHECK_INCLUDE_FILE("stdio.h" HAVE_STDIO_H)

CHECK_FUNCTION_EXISTS(gettimeofday HAVE_GETTIMEOFDAY)
CHECK_FUNCTION_EXISTS(usleep HAVE_USLEEP)
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

if(WIN32) #THOSE FILES ARE FUNCTIONS ARE NOT DETECTED BUT THEY SHOULD...
    set(HAVE_UCONTEXT_H 1)
    set(HAVE_MAKECONTEXT 1)
    set(HAVE_SNPRINTF 1)
    set(HAVE_VSNPRINTF 1)
endif(WIN32)

set(CONTEXT_UCONTEXT 0)
SET(CONTEXT_THREADS 0)
SET(HAVE_RUBY 0)
set(HAVE_LUA 0)
SET(HAVE_JAVA 0)
SET(HAVE_TRACING 0)

if(enable_tracing)
	SET(HAVE_TRACING 1)
endif(enable_tracing)

if(enable_latency_bound_tracking)
	SET(HAVE_LATENCY_BOUND_TRACKING 1)
else(enable_latency_bound_tracking)
  if(enable_gtnets)
    message("turning latency_bound_tracking to ON because GTNeTs is ON")
    SET(enable_latency_bound_tracking ON)
    SET(HAVE_LATENCY_BOUND_TRACKING 1)
  else(enable_gtnets)
    SET(HAVE_LATENCY_BOUND_TRACKING 0)
  endif(enable_gtnets)
endif(enable_latency_bound_tracking)

if(enable_model-checking AND HAVE_MMAP)
	SET(HAVE_MC 1)
	SET(MMALLOC_WANT_OVERIDE_LEGACY 1)
else(enable_model-checking AND HAVE_MMAP)
	SET(HAVE_MC 0)
	SET(MMALLOC_WANT_OVERIDE_LEGACY 0)
endif(enable_model-checking AND HAVE_MMAP)

if(enable_lua)
	include(FindLua51Simgrid)
	if(LUA51_FOUND)
		set(HAVE_LUA 1)
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-I${LUA_INCLUDE_DIR} ")
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-L${LUA_LIBRARY_DIR} ")
	else(LUA51_FOUND)
		message("Lua binding need version 5.1 and cmake version 2.8")
		message("Cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
	endif(LUA51_FOUND)
endif(enable_lua)

if(enable_ruby)
	include(FindRuby)
	if(RUBY_LIBRARY)
		if(RUBY_VERSION_MAJOR MATCHES "1" AND RUBY_VERSION_MINOR MATCHES "8")
			set(LIB_RUBY_VERSION "${RUBY_VERSION_MAJOR}.${RUBY_VERSION_MINOR}.${RUBY_VERSION_PATCH}")
			string(REGEX MATCH "ruby.*[0-9]" RUBY_LIBRARY_NAME ${RUBY_LIBRARY})
			if(NOT RUBY_LIBRARY_NAME)
				set(RUBY_LIBRARY_NAME ruby)
			endif(NOT RUBY_LIBRARY_NAME)
			string(REGEX REPLACE "/libruby.*$" "" RUBY_LIBRARY ${RUBY_LIBRARY})
			SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-I${RUBY_CONFIG_INCLUDE_DIR} ") #path to config.h
			string(COMPARE EQUAL "${RUBY_INCLUDE_DIR}" "${RUBY_CONFIG_INCLUDE_DIR}" operation)
			if(NOT operation)
				SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-I${RUBY_INCLUDE_DIR} ") #path to ruby.h
			endif(NOT operation)
			ADD_DEFINITIONS("-I${PROJECT_DIRECTORY}/src/bindings/ruby -I${PROJECT_DIRECTORY}/src/simix")
			SET(HAVE_RUBY 1)
		else(RUBY_VERSION_MAJOR MATCHES "1" AND RUBY_VERSION_MINOR MATCHES "8")
			message("Ruby binding need version 1.8.x actually version ${RUBY_VERSION_MAJOR}.${RUBY_VERSION_MINOR}.x")
			SET(HAVE_RUBY 0)
		endif(RUBY_VERSION_MAJOR MATCHES "1" AND RUBY_VERSION_MINOR MATCHES "8")
	else(RUBY_LIBRARY)
		SET(HAVE_RUBY 0)
	endif(RUBY_LIBRARY)
	
	if(NOT RUBY_EXECUTABLE)
		message("Take care : you don't have ruby executable so you can compile and build examples but can't execute them!!!") 
	endif(NOT RUBY_EXECUTABLE)
	
endif(enable_ruby)

#--------------------------------------------------------------------------------------------------
### Initialize of Smpi

if(enable_smpi)
	include(FindF2c)
	if(HAVE_F2C_H)
       string(REGEX MATCH "-I${HAVE_F2C_H} " operation "${CMAKE_C_FLAGS}")
	   if(NOT operation)
			SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-I${HAVE_F2C_H} ")
	   endif(NOT operation)
	else(HAVE_F2C_H)
		message("-- You should install f2c before use smpi.")
	endif(HAVE_F2C_H)	
endif(enable_smpi)

#--------------------------------------------------------------------------------------------------
### Initialize of CONTEXT JAVA

if(enable_java)
	include(FindJava)
	include(FindJNI)
	if(JAVA_INCLUDE_PATH)
		set(HAVE_JNI_H 1)
	endif(JAVA_INCLUDE_PATH)	
	if(JAVA_COMPILE AND JAVA_INCLUDE_PATH AND JAVA_INCLUDE_PATH2)
		SET(HAVE_JAVA 1)
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-I${JAVA_INCLUDE_PATH} ")
		if(NOT JAVA_INCLUDE_PATH STREQUAL JAVA_INCLUDE_PATH2)
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-I${JAVA_INCLUDE_PATH2} ")			
		endif(NOT JAVA_INCLUDE_PATH STREQUAL JAVA_INCLUDE_PATH2)
	else(JAVA_COMPILE AND JAVA_INCLUDE_PATH AND JAVA_INCLUDE_PATH2) 
		SET(HAVE_JAVA 0)
	endif(JAVA_COMPILE AND JAVA_INCLUDE_PATH AND JAVA_INCLUDE_PATH2)
endif(enable_java)

#--------------------------------------------------------------------------------------------------
### Initialize of CONTEXT GTNETS
if(NOT enable_gtnets OR enable_supernovae)
	SET(HAVE_GTNETS 0)
else(NOT enable_gtnets OR enable_supernovae)
	set(GTNETS_LDFLAGS "-L${gtnets_path}/lib")
	set(GTNETS_CPPFLAGS "-I${gtnets_path}/include/gtnets")
	exec_program("${CMAKE_CXX_COMPILER} ${GTNETS_CPPFLAGS} -lgtnets ${GTNETS_LDFLAGS} ${PROJECT_DIRECTORY}/buildtools/Cmake/test_prog/prog_gtnets.cpp " OUTPUT_VARIABLE COMPILE_GTNETS_VAR)	
	if(COMPILE_GTNETS_VAR)
		SET(HAVE_GTNETS 0)
	else(COMPILE_GTNETS_VAR)
		SET(HAVE_GTNETS 1)
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}${GTNETS_CPPFLAGS} ${GTNETS_LDFLAGS} ")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}${GTNETS_CPPFLAGS} ${GTNETS_LDFLAGS} ")
		string(REGEX MATCH "${gtnets_path}/lib" operation "$ENV{LD_LIBRARY_PATH}")
		if(NOT operation)
			message(FATAL_ERROR "\n\nTo use GTNETS don't forget to set LD_LIBRARY_PATH with \n\texport LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${gtnets_path}/lib\n\n")
		endif(NOT operation)
	endif(COMPILE_GTNETS_VAR)
endif(NOT enable_gtnets OR enable_supernovae)

#--------------------------------------------------------------------------------------------------
#--------------------------------------------------------------------------------------------------
### Initialize of pcre
find_library(PATH_PCRE_LIB pcre)
find_file(PATH_PCRE_H "pcre.h")
set(HAVE_PCRE_LIB 0)
if(PATH_PCRE_LIB AND PATH_PCRE_H)
       string(REGEX REPLACE "/libpcre.*[.]${LIB_EXE}$" "" PATHLIBPCRE "${PATH_PCRE_LIB}")
       string(REGEX REPLACE "/pcre.h" "" PATH_PCRE_H "${PATH_PCRE_H}")
       string(REGEX MATCH "-L${PATHLIBPCRE} " operation "${CMAKE_C_FLAGS}")
	   if(NOT operation)
			SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-L${PATHLIBPCRE} ")
	   endif(NOT operation)
	   string(REGEX MATCH "-I${PATH_PCRE_H} " operation "${CMAKE_C_FLAGS}")
	   if(NOT operation)
			SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}-I${PATH_PCRE_H} ")
	   endif(NOT operation)	   
       set(HAVE_PCRE_LIB 1)
else(PATH_PCRE_LIB)
		message("You should install libpcre (please install the libpcre3-dev package or equivalent)")
endif(PATH_PCRE_LIB AND PATH_PCRE_H)

#--------------------------------------------------------------------------------------------------
### Initialize of CONTEXT THREADS

if(pthread)
	set(pthread 1)
elseif(pthread)
	set(pthread 0)
endif(pthread)

if(pthread)
	### HAVE_SEM_INIT
  	
  	if(HAVE_SEM_INIT_LIB)
		exec_program("${CMAKE_C_COMPILER} -lpthread ${PROJECT_DIRECTORY}/buildtools/Cmake/test_prog/prog_sem_init.c" OUTPUT_VARIABLE HAVE_SEM_INIT_run)
	    	if(HAVE_SEM_INIT_run)
			set(HAVE_SEM_INIT 0)
	    	else(HAVE_SEM_INIT_run)
			set(HAVE_SEM_INIT 1)
		endif(HAVE_SEM_INIT_run)
  	endif(HAVE_SEM_INIT_LIB)

	### HAVE_SEM_TIMEDWAIT

	if(HAVE_SEM_TIMEDWAIT_LIB)
		exec_program("${CMAKE_C_COMPILER} -lpthread ${PROJECT_DIRECTORY}/buildtools/Cmake/test_prog/prog_sem_timedwait.c" OUTPUT_VARIABLE HAVE_SEM_TIMEDWAIT_run)
		if(HAVE_SEM_TIMEDWAIT_run)
			set(HAVE_SEM_TIMEDWAIT 0)
		else(HAVE_SEM_TIMEDWAIT_run)
			set(HAVE_SEM_TIMEDWAIT 1)
		endif(HAVE_SEM_TIMEDWAIT_run)
	endif(HAVE_SEM_TIMEDWAIT_LIB)

	### HAVE_MUTEX_TIMEDLOCK

	if(HAVE_MUTEX_TIMEDLOCK_LIB)
		exec_program("${CMAKE_C_COMPILER} -lpthread ${PROJECT_DIRECTORY}/buildtools/Cmake/test_prog/prog_mutex_timedlock.c" OUTPUT_VARIABLE HAVE_SEM_TIMEDWAIT_run)
		if(HAVE_MUTEX_TIMEDLOCK_run)
			set(HAVE_MUTEX_TIMEDLOCK 0)
		else(HAVE_MUTEX_TIMEDLOCK_run)
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
    if(__VISUALC__)
	set(mcsc_flags "/D_XBT_WIN32 /I${PROJECT_DIRECTORY}/include/xbt /I${PROJECT_DIRECTORY}/src/xbt")
	endif(__VISUALC__)
	if(__GNUC__)
		set(mcsc_flags "-D_XBT_WIN32 -I${PROJECT_DIRECTORY}/include/xbt -I${PROJECT_DIRECTORY}/src/xbt")
	endif(__GNUC__)
endif(WIN32)

IF(CMAKE_CROSSCOMPILING)
	IF(WIN32)
		set(windows_context "yes")
		set(IS_WINDOWS 1)	
	ENDIF(WIN32)
ELSE(CMAKE_CROSSCOMPILING)
	try_run(RUN_mcsc_VAR COMPILE_mcsc_VAR
		${simgrid_BINARY_DIR}
		${PROJECT_DIRECTORY}/buildtools/Cmake/test_prog/prog_AC_CHECK_MCSC.c
		COMPILE_DEFINITIONS "${mcsc_flags}"
		OUTPUT_VARIABLE var_compil
		)
		
		if(EXISTS "${simgrid_BINARY_DIR}/conftestval" AND COMPILE_mcsc_VAR)
			file(READ "${simgrid_BINARY_DIR}/conftestval" mcsc)
			STRING(REPLACE "\n" "" mcsc "${mcsc}")
			if(mcsc)
				set(mcsc "yes")
			elseif(mcsc)
				set(mcsc "no")
			endif(mcsc)
	        else(EXISTS "${simgrid_BINARY_DIR}/conftestval" AND COMPILE_mcsc_VAR)
			set(mcsc "no")
		endif(EXISTS "${simgrid_BINARY_DIR}/conftestval" AND COMPILE_mcsc_VAR)
ENDIF(CMAKE_CROSSCOMPILING)

if(mcsc MATCHES "no" AND pthread)
	if(HAVE_WINDOWS_H)
		set(windows_context "yes")
		set(IS_WINDOWS 1)
	elseif(HAVE_WINDOWS_H)
		message(FATAL_ERROR "no appropriate backend found")
	endif(HAVE_WINDOWS_H)
endif(mcsc MATCHES "no" AND pthread)

if(with_context MATCHES "ucontext" AND mcsc MATCHES "no")
	message(FATAL_ERROR "-Dwith-context=ucontext specified but ucontext unusable.")
endif(with_context MATCHES "ucontext" AND mcsc MATCHES "no")

set(with_context_ok 0)
if(with_context MATCHES "windows")
	set(with_context_ok 1)
	if(NOT HAVE_WINDOWS_H)
		message(FATAL_ERROR "no appropriate backend found windows")
	endif(NOT HAVE_WINDOWS_H)
endif(with_context MATCHES "windows")

if(with_context MATCHES "pthreads")
	set(with_context_ok 1)
	set(with_context "pthread")
endif(with_context MATCHES "pthreads")

if(with_context MATCHES "auto")
	set(with_context_ok 1)
	set(with_context "ucontext")
	message("with_context auto change to ucontext")
endif(with_context MATCHES "auto")

if(with_context MATCHES "ucontext")
	set(with_context_ok 1)
	if(mcsc)
		set(CONTEXT_UCONTEXT 1)
	else(mcsc)
		if(windows_context MATCHES "yes")
			set(with_context "windows")
			message("with_context ucontext change to windows")
		else(windows_context MATCHES "yes")
			set(with_context "pthread")
			message("with_context ucontext change to pthread")
		endif(windows_context MATCHES "yes")
	endif(mcsc)
endif(with_context MATCHES "ucontext")

if(with_context MATCHES "pthread")
	set(with_context_ok 1)
	if(NOT pthread)
		message(FATAL_ERROR "Cannot find pthreads (try -Dwith_context=ucontext if you haven't already tried).")
	endif(NOT pthread)
	SET(CONTEXT_THREADS 1)
endif(with_context MATCHES "pthread")

if(with_context MATCHES "ucontext")
	SET(CONTEXT_THREADS 0)
endif(with_context MATCHES "ucontext")

if(NOT with_context_ok)
	message(FATAL_ERROR "-Dwith-context must be either ucontext or pthread")
endif(NOT with_context_ok)

###############
## SVN version check
##
if(IS_DIRECTORY ${PROJECT_DIRECTORY}/.svn)
	find_file(SVN ".svn" ${PROJECT_DIRECTORY})
	exec_program("svnversion ${PROJECT_DIRECTORY}" OUTPUT_VARIABLE "SVN_VERSION")
	message("SVN_VERSION : ${SVN_VERSION}")
endif(IS_DIRECTORY ${PROJECT_DIRECTORY}/.svn)

if(IS_DIRECTORY ${PROJECT_DIRECTORY}/.git)

	exec_program("git --git-dir=${PROJECT_DIRECTORY}/.git log --oneline -1" OUTPUT_VARIABLE "GIT_VERSION")
	exec_program("git --git-dir=${PROJECT_DIRECTORY}/.git log -n 1 --format=%ai ." OUTPUT_VARIABLE "GIT_DATE")
	exec_program("git svn info" ${PROJECT_DIRECTORY} OUTPUT_VARIABLE "GIT_SVN_VERSION")
	
	string(REGEX REPLACE " .*" "" GIT_VERSION "${GIT_VERSION}")
	string(REPLACE "\n" ";" GIT_SVN_VERSION ${GIT_SVN_VERSION})
	STRING(REPLACE " +0000" "" GIT_DATE ${GIT_DATE})
	STRING(REPLACE " " "~" GIT_DATE ${GIT_DATE})
	STRING(REPLACE ":" "-" GIT_DATE ${GIT_DATE})
	foreach(line ${GIT_SVN_VERSION})
		string(REGEX MATCH "^Revision:.*" line_good ${line})
		if(line_good)
			string(REPLACE "Revision: " "" line_good ${line_good})
			set(SVN_VERSION ${line_good})
		endif(line_good)
	endforeach(line ${GIT_SVN_VERSION})
endif(IS_DIRECTORY ${PROJECT_DIRECTORY}/.git)

###################################
## SimGrid and GRAS specific checks
##

IF(NOT CMAKE_CROSSCOMPILING)
# Check architecture signature begin
try_run(RUN_GRAS_VAR COMPILE_GRAS_VAR
	${simgrid_BINARY_DIR}
	${PROJECT_DIRECTORY}/buildtools/Cmake/test_prog/prog_GRAS_ARCH.c
	RUN_OUTPUT_VARIABLE var1
	)
if(BIGENDIAN)
  set(val_big "B${var1}")
  set(GRAS_BIGENDIAN 1)
else(BIGENDIAN)
  set(val_big "l${var1}")
  set(GRAS_BIGENDIAN 0)
endif(BIGENDIAN)

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

if(val_big MATCHES "B_C:1/1:_I:2/2:4/4:4/4:8/8:_P:4/4:4/4:_D:4/4:8/8:") 
	#gras_arch=5; gras_size=32; gras_arch_name=big32;
	SET(GRAS_ARCH_32_BITS 1)
	SET(GRAS_THISARCH 5)
endif(val_big MATCHES "B_C:1/1:_I:2/2:4/4:4/4:8/8:_P:4/4:4/4:_D:4/4:8/8:")
if(val_big MATCHES "B_C:1/1:_I:2/2:4/4:4/4:8/8:_P:4/4:4/4:_D:4/4:8/4:") 
	#gras_arch=6; gras_size=32; gras_arch_name=big32_8_4;
	SET(GRAS_ARCH_32_BITS 1)
	SET(GRAS_THISARCH 6)
endif(val_big MATCHES "B_C:1/1:_I:2/2:4/4:4/4:8/8:_P:4/4:4/4:_D:4/4:8/4:")
if(val_big MATCHES "B_C:1/1:_I:2/2:4/4:4/4:8/4:_P:4/4:4/4:_D:4/4:8/4:") 
	#gras_arch=7; gras_size=32; gras_arch_name=big32_4;
	SET(GRAS_ARCH_32_BITS 1)
	SET(GRAS_THISARCH 7)
endif(val_big MATCHES "B_C:1/1:_I:2/2:4/4:4/4:8/4:_P:4/4:4/4:_D:4/4:8/4:")
if(val_big MATCHES "B_C:1/1:_I:2/2:4/2:4/2:8/2:_P:4/2:4/2:_D:4/2:8/2:") 
	#gras_arch=8; gras_size=32; gras_arch_name=big32_2;
	SET(GRAS_ARCH_32_BITS 1)
	SET(GRAS_THISARCH 8)
endif(val_big MATCHES "B_C:1/1:_I:2/2:4/2:4/2:8/2:_P:4/2:4/2:_D:4/2:8/2:") 
if(val_big MATCHES "B_C:1/1:_I:2/2:4/4:8/8:8/8:_P:8/8:8/8:_D:4/4:8/8:") 
	#gras_arch=9; gras_size=64; gras_arch_name=big64;
	SET(GRAS_ARCH_32_BITS 0)
	SET(GRAS_THISARCH 9)
endif(val_big MATCHES "B_C:1/1:_I:2/2:4/4:8/8:8/8:_P:8/8:8/8:_D:4/4:8/8:")
if(val_big MATCHES "B_C:1/1:_I:2/2:4/4:8/8:8/8:_P:8/8:8/8:_D:4/4:8/4:") 
	#gras_arch=10;gras_size=64; gras_arch_name=big64_8_4;
	SET(GRAS_ARCH_32_BITS 0)
	SET(GRAS_THISARCH 10)
endif(val_big MATCHES "B_C:1/1:_I:2/2:4/4:8/8:8/8:_P:8/8:8/8:_D:4/4:8/4:") 


# Check architecture signature end
try_run(RUN_GRAS_VAR COMPILE_GRAS_VAR
	${simgrid_BINARY_DIR}
	${PROJECT_DIRECTORY}/buildtools/Cmake/test_prog/prog_GRAS_CHECK_STRUCT_COMPACTION.c
	RUN_OUTPUT_VARIABLE var2
	)
separate_arguments(var2)
foreach(var_tmp ${var2})
	set(${var_tmp} 1)
endforeach(var_tmp ${var2})

# Check for [SIZEOF_MAX]
try_run(RUN_SM_VAR COMPILE_SM_VAR
	${simgrid_BINARY_DIR}
	${PROJECT_DIRECTORY}/buildtools/Cmake/test_prog/prog_max_size.c
	RUN_OUTPUT_VARIABLE var3
	)
SET(SIZEOF_MAX ${var3})
ENDIF(NOT CMAKE_CROSSCOMPILING)

#--------------------------------------------------------------------------------------------------

set(makecontext_CPPFLAGS_2 "")
if(HAVE_MAKECONTEXT OR WIN32)
	set(makecontext_CPPFLAGS "-DTEST_makecontext")
	if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
		set(makecontext_CPPFLAGS_2 "-DOSX")
	endif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	
    if(WIN32 AND __VISUALC__)
        set(makecontext_CPPFLAGS "/DTEST_makecontext")
	    set(makecontext_CPPFLAGS_2 "/D_XBT_WIN32 /I${PROJECT_DIRECTORY}/include/xbt /I${PROJECT_DIRECTORY}/src/xbt")
	endif(WIN32 AND __VISUALC__)
	if(WIN32 AND __GNUC__)
	    set(makecontext_CPPFLAGS "-DTEST_makecontext")
	    set(makecontext_CPPFLAGS_2 "-D_XBT_WIN32 -I${PROJECT_DIRECTORY}/include/xbt -I${PROJECT_DIRECTORY}/src/xbt")
	endif(WIN32 AND __GNUC__)
	
	try_run(RUN_makecontext_VAR COMPILE_makecontext_VAR
		${simgrid_BINARY_DIR}
		${PROJECT_DIRECTORY}/buildtools/Cmake/test_prog/prog_stacksetup.c
		COMPILE_DEFINITIONS "${makecontext_CPPFLAGS} ${makecontext_CPPFLAGS_2}"
		)
	file(READ ${simgrid_BINARY_DIR}/conftestval MAKECONTEXT_ADDR_SIZE)
	string(REPLACE "\n" "" MAKECONTEXT_ADDR_SIZE "${MAKECONTEXT_ADDR_SIZE}")
	string(REGEX MATCH ;^.*,;MAKECONTEXT_ADDR "${MAKECONTEXT_ADDR_SIZE}")
	string(REGEX MATCH ;,.*$; MAKECONTEXT_SIZE "${MAKECONTEXT_ADDR_SIZE}")
	string(REPLACE "," "" makecontext_addr "${MAKECONTEXT_ADDR}")
	string(REPLACE "," "" makecontext_size "${MAKECONTEXT_SIZE}")	
	set(pth_skaddr_makecontext "#define pth_skaddr_makecontext(skaddr,sksize) (${makecontext_addr})")
	set(pth_sksize_makecontext "#define pth_sksize_makecontext(skaddr,sksize) (${makecontext_size})")
	
endif(HAVE_MAKECONTEXT OR WIN32)

#--------------------------------------------------------------------------------------------------

### check for stackgrowth
if (NOT CMAKE_CROSSCOMPILING)
	try_run(RUN_makecontext_VAR COMPILE_makecontext_VAR
		${simgrid_BINARY_DIR}
		${PROJECT_DIRECTORY}/buildtools/Cmake/test_prog/prog_stackgrowth.c
		)
file(READ "${simgrid_BINARY_DIR}/conftestval" stack)
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
	${simgrid_BINARY_DIR}
	${PROJECT_DIRECTORY}/buildtools/Cmake/test_prog/prog_printf_null.c
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
	write_file("${PROJECT_DIRECTORY}/buildtools/Cmake/test_prog/prog_va_copy.c" "#include <stdlib.h>
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
	int main(int argc, char *argv[])
	{
	    test("test", 1, 2, 3, 4, 5, 6, 7, 8, 9);
	    exit(0);
	}"
	)
	try_compile(COMPILE_VA_NULL_VAR
	${simgrid_BINARY_DIR}
	${PROJECT_DIRECTORY}/buildtools/Cmake/test_prog/prog_va_copy.c
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
	${simgrid_BINARY_DIR}
	${PROJECT_DIRECTORY}/buildtools/Cmake/test_prog/prog_getline.c
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
	  	${simgrid_BINARY_DIR}
		${PROJECT_DIRECTORY}/buildtools/Cmake/test_prog/prog_snprintf.c
	    )	
	endif(CMAKE_CROSSCOMPILING)

	if(CMAKE_CROSSCOMPILING)
		set(RUN_VSNPRINTF_FUNC "cross")
		set(PREFER_PORTABLE_VSNPRINTF 1)
	else(CMAKE_CROSSCOMPILING)
  	   try_run(RUN_VSNPRINTF_FUNC_VAR COMPILE_VSNPRINTF_FUNC_VAR
		${simgrid_BINARY_DIR}
		${PROJECT_DIRECTORY}/buildtools/Cmake/test_prog/prog_vsnprintf.c
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

### File to create

configure_file("${PROJECT_DIRECTORY}/src/context_sysv_config.h.in" 			"${CMAKE_CURRENT_BINARY_DIR}/src/context_sysv_config.h" @ONLY IMMEDIATE)

SET( CMAKEDEFINE "#cmakedefine" )
configure_file("${PROJECT_DIRECTORY}/buildtools/Cmake/gras_config.h.in" 	"${CMAKE_CURRENT_BINARY_DIR}/src/gras_config.h" @ONLY IMMEDIATE)
configure_file("${CMAKE_CURRENT_BINARY_DIR}/src/gras_config.h" 			"${CMAKE_CURRENT_BINARY_DIR}/src/gras_config.h" @ONLY IMMEDIATE)
configure_file("${PROJECT_DIRECTORY}/include/simgrid_config.h.in" 		"${CMAKE_CURRENT_BINARY_DIR}/include/simgrid_config.h" @ONLY IMMEDIATE)

set(top_srcdir "${PROJECT_DIRECTORY}")
set(srcdir "${PROJECT_DIRECTORY}/src")

set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(includedir ${CMAKE_INSTALL_PREFIX}/include)
set(top_builddir ${PROJECT_DIRECTORY})
set(libdir ${exec_prefix}/lib)
set(CMAKE_LINKARGS "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_SMPI_COMMAND "export LD_LIBRARY_PATH=${CMAKE_CURRENT_BINARY_DIR}/lib:${gtnets_path}/lib:$LD_LIBRARY_PATH")

configure_file(${PROJECT_DIRECTORY}/include/smpi/smpif.h.in ${PROJECT_DIRECTORY}/include/smpi/smpif.h @ONLY)
configure_file(${PROJECT_DIRECTORY}/src/smpi/smpicc.in ${CMAKE_CURRENT_BINARY_DIR}/bin/smpicc @ONLY)
configure_file(${PROJECT_DIRECTORY}/src/smpi/smpif2c.in ${CMAKE_CURRENT_BINARY_DIR}/bin/smpif2c @ONLY)
configure_file(${PROJECT_DIRECTORY}/src/smpi/smpiff.in ${CMAKE_CURRENT_BINARY_DIR}/bin/smpiff @ONLY)
configure_file(${PROJECT_DIRECTORY}/src/smpi/smpirun.in ${CMAKE_CURRENT_BINARY_DIR}/bin/smpirun @ONLY)
configure_file(${PROJECT_DIRECTORY}/examples/smpi/hostfile ${CMAKE_CURRENT_BINARY_DIR}/examples/smpi/hostfile COPYONLY)
configure_file(${PROJECT_DIRECTORY}/examples/msg/small_platform.xml ${CMAKE_CURRENT_BINARY_DIR}/examples/msg/small_platform.xml COPYONLY)
configure_file(${PROJECT_DIRECTORY}/examples/msg/small_platform_with_routers.xml ${CMAKE_CURRENT_BINARY_DIR}/examples/msg/small_platform_with_routers.xml COPYONLY)

exec_program("chmod a=rwx ${CMAKE_CURRENT_BINARY_DIR}/bin/smpicc" OUTPUT_VARIABLE OKITOKI)
exec_program("chmod a=rwx ${CMAKE_CURRENT_BINARY_DIR}/bin/smpif2c" OUTPUT_VARIABLE OKITOKI)
exec_program("chmod a=rwx ${CMAKE_CURRENT_BINARY_DIR}/bin/smpiff" OUTPUT_VARIABLE OKITOKI)
exec_program("chmod a=rwx ${CMAKE_CURRENT_BINARY_DIR}/bin/smpirun" OUTPUT_VARIABLE OKITOKI)
