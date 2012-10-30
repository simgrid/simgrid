# Configure CTest. For details, see:
# http://www.cmake.org/Wiki/CMake_Testing_With_CTest#Customizing_CTest

if(APPLE)
  SET(BUILDNAME "APPLE" CACHE TYPE INTERNAL FORCE)
else()
  SET(BUILDNAME "UNIX" CACHE TYPE INTERNAL FORCE)
  if(WIN32)
    SET(BUILDNAME "WINDOWS" CACHE TYPE INTERNAL FORCE)
  endif()
endif()

if(enable_memcheck)
  set(CTEST_TIMEOUT "300") #TIMEOUT FOR EACH TEST
endif()

if(enable_compile_warnings AND enable_compile_optimizations)
  SET(BUILDNAME "FULL_FLAGS" CACHE TYPE INTERNAL FORCE)
endif()

if(enable_supernovae)
  SET(BUILDNAME "SUPERNOVAE" CACHE TYPE INTERNAL FORCE)
endif()

if(HAVE_GTNETS)
  SET(BUILDNAME "GTNETS" CACHE TYPE INTERNAL FORCE)
endif()

if(HAVE_MC)
  SET(BUILDNAME "MODEL-CHECKING" CACHE TYPE INTERNAL FORCE)
endif()

if(enable_memcheck)
  SET(BUILDNAME "MEMCHECK" CACHE TYPE INTERNAL FORCE)
endif()

set(osname ${CMAKE_SYSTEM_NAME})
set(cpu ${CMAKE_SYSTEM_PROCESSOR})
set(DISTRIB2 ${CMAKE_SYSTEM_VERSION})

SET(SITE "${osname}_${DISTRIB2}_${cpu}")
SET(CTEST_SITE "${osname}_${DISTRIB2}_${cpu}")
SET(CTEST_PROJECT_NAME "${PROJECT_NAME}")
SET(CTEST_CUSTOM_MAXIMUM_FAILED_TEST_OUTPUT_SIZE "3000000")
SET(CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE "3000000")

set(PIPOL_IMAGE $ENV{PIPOL_IMAGE})
if(NOT ${PIPOL_IMAGE} MATCHES "\n")
  set(SITE ${PIPOL_IMAGE})
endif()

set(PATTERN_CTEST_IGNORED "")
if(enable_coverage)
    set(PATTERN_CTEST_IGNORED 
      "/tools/"
      "/buildtools/"
      "/include/"
      "/testsuite/"
      "/teshsuite/"
      "/src/bindings/"
    )
    if(release)
       set(PATTERN_CTEST_IGNORED 
        ${PATTERN_CTEST_IGNORED}
        "/examples/"
        )
    endif()
endif()

CONFIGURE_FILE(${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/CTestCustom.cmake ${CMAKE_BINARY_DIR}/CTestCustom.cmake @ONLY)
