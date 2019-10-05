# Configure CTest. For details, see:
# http://www.cmake.org/Wiki/CMake_Testing_With_CTest#Customizing_CTest

if(APPLE)
  SET(BUILDNAME "APPLE" CACHE INTERNAL "Buildname" FORCE)
else()
  SET(BUILDNAME "UNIX" CACHE INTERNAL "Buildname" FORCE)
  if(WIN32)
    SET(BUILDNAME "WINDOWS" CACHE INTERNAL "Buildname" FORCE)
  endif()
endif()

if(NOT enable_memcheck)
  set(DART_TESTING_TIMEOUT "500") #TIMEOUT FOR EACH TEST
else()
  set(DART_TESTING_TIMEOUT "3000") #TIMEOUT FOR EACH TEST
endif()

if(enable_compile_warnings AND enable_compile_optimizations)
  SET(BUILDNAME "FULL_FLAGS" CACHE INTERNAL "Buildname" FORCE)
endif()

if(SIMGRID_HAVE_MC)
  SET(BUILDNAME "MODEL-CHECKING" CACHE INTERNAL "Buildname" FORCE)
endif()

if(enable_memcheck)
  SET(BUILDNAME "MEMCHECK" CACHE INTERNAL "Buildname" FORCE)
endif()

set(osname ${CMAKE_SYSTEM_NAME})
set(cpu ${CMAKE_SYSTEM_PROCESSOR})
set(DISTRIB2 ${CMAKE_SYSTEM_VERSION})

SET(SITE "${osname}_${DISTRIB2}_${cpu}")
SET(CTEST_SITE "${osname}_${DISTRIB2}_${cpu}")
SET(CTEST_PROJECT_NAME "${PROJECT_NAME}")
SET(CTEST_CUSTOM_MAXIMUM_FAILED_TEST_OUTPUT_SIZE "3000000")
SET(CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE "3000000")

set(PATTERN_CTEST_IGNORED "")
if(enable_coverage)
    set(PATTERN_CTEST_IGNORED
      "/tools/"
      "/buildtools/"
      "/include/"
      "/teshsuite/"
      "/src/bindings/"
      "/examples/"
    )
endif()

CONFIGURE_FILE(${CMAKE_HOME_DIRECTORY}/tools/cmake/CTestCustom.cmake ${CMAKE_BINARY_DIR}/CTestCustom.cmake @ONLY)
