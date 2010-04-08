# Configure CTest. For details, see:
# http://www.cmake.org/Wiki/CMake_Testing_With_CTest#Customizing_CTest

#Get the hostname of current machine :
exec_program(${HOSTNAME_CMD} OUTPUT_VARIABLE HOSTNAME)
set(SITE "${HOSTNAME}")
MARK_AS_ADVANCED(HOSTNAME_CMD)

#Get the system information of current machine
macro(getuname name flag)
exec_program("${UNAME}" ARGS "${flag}" OUTPUT_VARIABLE "${name}")
endmacro(getuname)
MARK_AS_ADVANCED(UNAME)

exec_program("${CAT}" ARGS "version" OUTPUT_VARIABLE VERSION)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  SET(DISTRIB2 "OSX")
else(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  #Try to get the distrib
  exec_program("${CAT}" ARGS " /etc/issue" OUTPUT_VARIABLE DISTRIB)
  MARK_AS_ADVANCED(CAT)
  STRING(REPLACE " \\n \\l" "" DISTRIB2 ${DISTRIB})
endif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  
getuname(osname -s)
getuname(node -n)
getuname(osrel  -r)
getuname(cpu    -m)
	
#SET(BUILDNAME "${osname}_${DISTRIB2}_${cpu}")
#SET(CTEST_SITE "${node}")
SET(BUILDNAME "none" CACHE TYPE INTERNAL FORCE)

if(with_context MATCHES ucontext AND NOT supernovae)
	SET(BUILDNAME "UCONTEXT" CACHE TYPE INTERNAL FORCE)
endif(with_context MATCHES ucontext AND NOT supernovae)

if(with_context MATCHES pthread AND NOT supernovae)
	SET(BUILDNAME "PTHREAD" CACHE TYPE INTERNAL FORCE)
endif(with_context MATCHES pthread AND NOT supernovae)

if(enable_compile_warnings AND enable_compile_optimizations)
	SET(BUILDNAME "FULL_FLAGS" CACHE TYPE INTERNAL FORCE)
endif(enable_compile_warnings AND enable_compile_optimizations)

if(supernovae)
	SET(BUILDNAME "SUPERNOVAE" CACHE TYPE INTERNAL FORCE)
endif(supernovae)

if(NOT disable_gtnets AND HAVE_GTNETS)
	SET(BUILDNAME "GTNETS" CACHE TYPE INTERNAL FORCE)
endif(NOT disable_gtnets AND HAVE_GTNETS)

if(HAVE_TRACING)
	SET(BUILDNAME "TRACING" CACHE TYPE INTERNAL FORCE)
endif(HAVE_TRACING)

if(enable_memcheck)
	SET(BUILDNAME "MEMCHECK" CACHE TYPE INTERNAL FORCE)
endif(enable_memcheck)

SET(SITE "${osname}_${DISTRIB2}_${cpu}")
SET(CTEST_SITE "${osname}_${DISTRIB2}_${cpu}")
SET(CTEST_PROJECT_NAME "${PROJECT_NAME}")
SET(CTEST_CUSTOM_MAXIMUM_FAILED_TEST_OUTPUT_SIZE "3000000")
SET(CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE "3000000")

string(REPLACE "/" "" SITE ${SITE})
exec_program("echo $PIPOL_IMAGE" OUTPUT_VARIABLE PIPOL_IMAGE)
#message("PIPOL_IMAGE : \"${PIPOL_IMAGE}\"")
if(NOT ${PIPOL_IMAGE} MATCHES "\n")
set(SITE ${PIPOL_IMAGE})
endif(NOT ${PIPOL_IMAGE} MATCHES "\n")
