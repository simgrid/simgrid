# Configure CTest. For details, see:
# http://www.cmake.org/Wiki/CMake_Testing_With_CTest#Customizing_CTest

SET(BUILDNAME "none" CACHE TYPE INTERNAL FORCE)
if(enable_memcheck)
	set(CTEST_TIMEOUT "300") #TIMEOUT FOR EACH TEST
endif(enable_memcheck)
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

if(HAVE_GTNETS)
	SET(BUILDNAME "GTNETS" CACHE TYPE INTERNAL FORCE)
endif(HAVE_GTNETS)

if(HAVE_TRACING)
	SET(BUILDNAME "TRACING" CACHE TYPE INTERNAL FORCE)
endif(HAVE_TRACING)

if(HAVE_MC)
	SET(BUILDNAME "MODEL-CHECKING" CACHE TYPE INTERNAL FORCE)
endif(HAVE_MC)

if(enable_memcheck)
	SET(BUILDNAME "MEMCHECK" CACHE TYPE INTERNAL FORCE)
endif(enable_memcheck)

if(WIN32)
	SET(BUILDNAME "WINDOWS" CACHE TYPE INTERNAL FORCE)
endif(WIN32)

set(osname ${CMAKE_SYSTEM_NAME})
set(cpu ${CMAKE_SYSTEM_PROCESSOR})
set(DISTRIB2 ${CMAKE_SYSTEM_VERSION})

SET(SITE "${osname}_${DISTRIB2}_${cpu}")
SET(CTEST_SITE "${osname}_${DISTRIB2}_${cpu}")
SET(CTEST_PROJECT_NAME "${PROJECT_NAME}")
SET(CTEST_CUSTOM_MAXIMUM_FAILED_TEST_OUTPUT_SIZE "3000000")
SET(CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE "3000000")

exec_program("echo $PIPOL_IMAGE" OUTPUT_VARIABLE PIPOL_IMAGE)
#message("PIPOL_IMAGE : \"${PIPOL_IMAGE}\"")
if(NOT ${PIPOL_IMAGE} MATCHES "\n")
set(SITE ${PIPOL_IMAGE})
endif(NOT ${PIPOL_IMAGE} MATCHES "\n")
