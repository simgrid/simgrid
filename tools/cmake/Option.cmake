### ARGs use -D[var]=[ON/OFF] or [1/0] or [true/false](see below)
### ex: cmake -Denable_java=ON -Denable_ns3=ON ./

set(BIBTEX2HTML ${BIBTEX2HTML} CACHE PATH "Path to bibtex2html")

if(NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local/simgrid/" CACHE PATH "Path where this project should be installed")
else()
  set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE PATH "Path where this project should be installed")
endif()

option(release "Whether Release Mode is activated (disable tests on experimental parts)" on)

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
option(enable_documentation "Whether to produce documentation" on)

option(enable_ns3            "Whether ns3 model is activated." off)
option(enable_java           "Whether the Java bindings are activated." off)
option(enable_lib_in_jar     "Whether the native libraries are bundled in a Java jar file" on)

option(enable_lua            "Whether the Lua bindings are activated." off)

option(enable_model-checking "Turn this on to experiment with our prototype of model-checker (hinders the simulation's performance even if turned off at runtime)" off)
option(enable_jedule         "Jedule output of SimDAG." off)

if(WIN32)
  option(enable_smpi "Whether SMPI in included in library." off)
  option(enable_smpi_MPICH3_testsuite "Whether the test suite form MPICH 3 should be built" off)
else()
  option(enable_smpi "Whether SMPI in included in library." on)
  option(enable_smpi_MPICH3_testsuite "Whether the test suite form MPICH 3 should be built" off)
endif()
option(enable_smpi_ISP_testsuite "Whether the test suite from ISP should be built." off)


# Internal targets used by jenkins
###
option(enable_coverage "Whether coverage should be enabled." off)
mark_as_advanced(enable_coverage)
option(enable_memcheck "Enable memcheck." off)
mark_as_advanced(enable_memcheck)
option(enable_memcheck_xml "Enable memcheck with xml output." off)
mark_as_advanced(enable_memcheck_xml)

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
