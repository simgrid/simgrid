### ARGs use -D[var]=[ON/OFF] or [1/0] or [true/false](see below)
### ex: cmake -Denable_java=ON -Denable_ns3=ON ./

set(BIBTEX2HTML ${BIBTEX2HTML} CACHE PATH "Path to bibtex2html")

if(NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local/simgrid/" CACHE PATH "Path where this project should be installed")
else()
  set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE PATH "Path where this project should be installed")
endif()

# How to build
###
option(enable_compile_optimizations "Whether to produce efficient code for the SimGrid library" on)
option(enable_compile_warnings      "Whether compilation warnings should be turned into errors." off)
option(enable_lto                   "Whether we should try to activate the LTO (link time optimisation)" on)
option(enable_mallocators           "Enable mallocators (disable only for debugging purpose)." on)
option(enable_maintainer_mode       "Whether flex and flexml files should be rebuilt." off)
option(enable_debug                 "Turn this off to remove all debug messages at compile time (faster, but no debug activatable)" on)

# Optional modules
###
option(enable_documentation "Whether to produce documentation" off)

option(enable_ns3            "Whether ns-3 model is activated." off)
option(enable_java           "Whether the Java bindings are activated." off)
option(enable_msg            "Whether the MSG module is activated." off)
option(enable_lib_in_jar     "Whether the native libraries are bundled in a Java jar file" on)
option(minimal-bindings      "Whether to compile the bindings libraries (Java/Python) with the minimal dependency set" off)
mark_as_advanced(minimal-bindings)
if(minimal-bindings)
  set(enable_lib_in_jar on)
endif()

option(enable_lua            "Whether the Lua bindings are activated." off)

option(enable_model-checking "Turn this on to experiment with our prototype of model-checker (hinders the simulation's performance even if turned off at runtime)" off)
option(enable-model-checking "Please set 'enable_model-checking' instead" off)
mark_as_advanced(enable-model-checking)
if(enable-model-checking)
  SET(enable_model-checking ON CACHE BOOL "Whether to compile the model-checker" FORCE)
endif()

if(WIN32)
  option(enable_smpi "Whether SMPI is included in the library." off)
else()
  option(enable_smpi "Whether SMPI is included in the library." on)
  # PAPI does not support windows (they did in 3.7, but not anymore in 5.x)
  # See http://icl.cs.utk.edu/papi/custom/index.html?lid=62&slid=96
  option(enable_smpi_papi    "Whether SMPI supports PAPI bindings." off)
endif()
option(enable_smpi_MPICH3_testsuite "Whether the test suite form MPICH 3 should be built" off)
option(enable_smpi_ISP_testsuite "Whether the test suite from ISP should be built." off)

# Internal targets used by jenkins
###
option(enable_fortran "Whether fortran is used with SMPI. Turned on by default if gfortran is found." on)
option(enable_coverage "Whether coverage should be enabled." off)
mark_as_advanced(enable_coverage)
option(enable_memcheck "Enable memcheck." off)
mark_as_advanced(enable_memcheck)
option(enable_memcheck_xml "Enable memcheck with xml output." off)
mark_as_advanced(enable_memcheck_xml)
option(enable_address_sanitizer "Whether address sanitizer is turned on." off)
mark_as_advanced(enable_address_sanitizer)
option(enable_thread_sanitizer "Whether thread sanitizer is turned on." off)
mark_as_advanced(enable_thread_sanitizer)
option(enable_undefined_sanitizer "Whether undefined sanitizer is turned on." off)
mark_as_advanced(enable_undefined_sanitizer)

# Cmake, Y U NO hide your garbage??
###
mark_as_advanced(HAVE_SSH)
mark_as_advanced(HAVE_RSYNC)
mark_as_advanced(BIBTEX2HTML_PATH)
mark_as_advanced(BUILDNAME)
mark_as_advanced(ADDR2LINE)
mark_as_advanced(BIBTOOL_PATH)
mark_as_advanced(BUILD_TESTING)
mark_as_advanced(CMAKE_BUILD_TYPE)
mark_as_advanced(DART_ROOT)
mark_as_advanced(DOXYGEN_PATH)
mark_as_advanced(FIG2DEV_PATH)
mark_as_advanced(FLEXML_EXE)
mark_as_advanced(FLEX_EXE)
mark_as_advanced(GCOV_PATH)
mark_as_advanced(ICONV_PATH)
mark_as_advanced(MAKE_PATH)
mark_as_advanced(SVN)
mark_as_advanced(GIT)
mark_as_advanced(CMAKE_OSX_ARCHITECTURES)
mark_as_advanced(CMAKE_OSX_DEPLOYMENT_TARGET)
mark_as_advanced(CMAKE_OSX_SYSROOT)
mark_as_advanced(SED_EXE)
mark_as_advanced(BIBTEX2HTML)
mark_as_advanced(CMAKE_C_LINK_FLAGS)
mark_as_advanced(CMAKE_CXX_FLAGS)
mark_as_advanced(CMAKE_Fortran_LINK_FLAGS)
