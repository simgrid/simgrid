cmake_minimum_required(VERSION 2.6)
PROJECT(SimgridProject)

# --------------------- OPTIONS OF THE PROJECT -----------------------------
OPTION( BUILD_SHARED_LIBS "Build SimGrid with shared libraries." ON )
OPTION( SIMGRID_BUILD_EXAMPLES "Build SimGrid examples." ON )
OPTION( SIMGRID_BUILD_DOCUMENTATION "Build the users manual documentation." OFF )
OPTION( SIMGRID_WITH_PTHREAD "Use pthread as context support." OFF )

OPTION( SIMGRID_USE_GTNETS "Build SimGrid with GTNets support." OFF )

### Check 32bits or 64bits
INCLUDE (CheckTypeSize)
CHECK_TYPE_SIZE("int" SIZEOF_INT)
IF(SIZEOF_INT EQUAL 4)
  SET(ARCH_32_BITS 1)
	message("ARCH_32_BITS")
ELSE(SIZEOF_INT EQUAL 4)
  SET(ARCH_32_BITS 0)
	message("ARCH_64_BITS")
ENDIF(SIZEOF_INT EQUAL 4)


### Enable tests
INCLUDE(CTestConfig.cmake)
ENABLE_TESTING()
INCLUDE(CTest)
INCLUDE(Dart)
INCLUDE(CTestList.txt)

# Install the CTest configuration in the build dir
configure_file(CTestConfig.cmake ${CMAKE_BINARY_DIR}/CTestCustom.ctest COPYONLY)

#SET (CMAKE_COMPILER_IS_GNUCXX 1)
#SET (CMAKE_COMPILER_IS_GNUCC 1)

