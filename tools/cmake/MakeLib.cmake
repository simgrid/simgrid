### Make Libs

# On Mac OSX, specify that rpath is useful to look for the dependencies
# See https://cmake.org/Wiki/CMake_RPATH_handling and Java.cmake
set(MACOSX_RPATH ON)
if(APPLE)
  SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE) # When installed, use system path
  set(CMAKE_SKIP_BUILD_RPATH FALSE)         # When executing from build tree, take the lib from the build path if exists
  set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) # When executing from build tree, take the lib from the system path if exists
  
  # add the current location of libsimgrid-java.dynlib as a location for libsimgrid.dynlib
  # (useful when unpacking the native libraries from the jarfile)
  set(CMAKE_INSTALL_RPATH "@loader_path/.;@rpath/.")
endif()

###############################
# Declare the library content #
###############################

# Actually declare our libraries
add_library(simgrid SHARED ${simgrid_sources})
set_target_properties(simgrid PROPERTIES VERSION ${libsimgrid_version})

add_dependencies(simgrid maintainer_files)

if(enable_model-checking)
  add_executable(simgrid-mc ${MC_SIMGRID_MC_SRC})
  target_link_libraries(simgrid-mc simgrid)
  set_target_properties(simgrid-mc
    PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

# Compute the dependencies of SimGrid
#####################################
if (HAVE_BOOST_CONTEXTS)
  set(SIMGRID_DEP "${SIMGRID_DEP} ${Boost_CONTEXT_LIBRARY}")
endif()

if(CMAKE_USE_PTHREADS_INIT AND ${HAVE_THREAD_CONTEXTS})
  set(SIMGRID_DEP "${SIMGRID_DEP} ${CMAKE_THREAD_LIBS_INIT}")
endif()

if(SIMGRID_HAVE_LUA)
  ADD_CUSTOM_TARGET(link_simgrid_lua ALL
    DEPENDS 	simgrid
    )
  SET(SIMGRID_DEP "${SIMGRID_DEP} ${LUA_LIBRARY} ${DL_LIBRARY}")
endif()

if(HAVE_PAPI)
  SET(SIMGRID_DEP "${SIMGRID_DEP} -lpapi")
endif()

if(HAVE_GRAPHVIZ)
  if(HAVE_CGRAPH_LIB)
    SET(SIMGRID_DEP "${SIMGRID_DEP} -lcgraph")
  else()
    if(HAVE_AGRAPH_LIB)
      SET(SIMGRID_DEP "${SIMGRID_DEP} -lagraph -lcdt")
    endif()
  endif()
endif()

if(SIMGRID_HAVE_MC AND HAVE_GNU_LD AND NOT ${DL_LIBRARY} STREQUAL "")
  SET(SIMGRID_DEP "${SIMGRID_DEP} ${DL_LIBRARY}")
endif()

if(SIMGRID_HAVE_NS3)
  SET(SIMGRID_DEP "${SIMGRID_DEP} -lns${NS3_VERSION}-core${NS3_SUFFIX} -lns${NS3_VERSION}-csma${NS3_SUFFIX} -lns${NS3_VERSION}-point-to-point${NS3_SUFFIX} -lns${NS3_VERSION}-internet${NS3_SUFFIX} -lns${NS3_VERSION}-network${NS3_SUFFIX} -lns${NS3_VERSION}-applications${NS3_SUFFIX}")
endif()

if(HAVE_POSIX_GETTIME)
  SET(SIMGRID_DEP "${SIMGRID_DEP} -lrt")
endif()

if("${CMAKE_SYSTEM}" MATCHES "FreeBSD")
  set(SIMGRID_DEP "${SIMGRID_DEP} -lprocstat")
endif()

# Compute the dependencies of SMPI
##################################

if(enable_smpi)
  if(NOT ${DL_LIBRARY} STREQUAL "")
    set(SIMGRID_DEP "${SIMGRID_DEP} ${DL_LIBRARY}") # for privatization
  endif()
  add_executable(smpimain src/smpi/smpi_main.c)
  target_link_libraries(smpimain simgrid)
  set_target_properties(smpimain
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

if(enable_smpi AND APPLE)
  set(SIMGRID_DEP "${SIMGRID_DEP} -Wl,-U -Wl,_smpi_simulated_main")
endif()

# See https://github.com/HewlettPackard/foedus_code/blob/master/foedus-core/cmake/FindGccAtomic.cmake
FIND_LIBRARY(GCCLIBATOMIC_LIBRARY NAMES atomic atomic.so.1 libatomic.so.1
  HINTS
    $ENV{HOME}/local/lib64
    $ENV{HOME}/local/lib
    /usr/local/lib64
    /usr/local/lib
    /opt/local/lib64
    /opt/local/lib
    /usr/lib64
    /usr/lib
    /lib64
    /lib
)

# Fix a FTBFS on armel, mips, mipsel and friends (Debian's #872881)
if(CMAKE_COMPILER_IS_GNUCC AND GCCLIBATOMIC_LIBRARY)
    set(SIMGRID_DEP   "${SIMGRID_DEP}   -Wl,--as-needed -latomic -Wl,--no-as-needed")
endif()

target_link_libraries(simgrid 	${SIMGRID_DEP})

# Dependencies from maintainer mode
###################################
if(enable_maintainer_mode)
  add_dependencies(simgrid smpi_generated_headers_call_location_tracing)
endif()
if(enable_maintainer_mode AND PYTHON_EXE)
  add_dependencies(simgrid simcalls_generated_src)
endif()
if(enable_maintainer_mode AND BISON_EXE AND LEX_EXE)
  add_dependencies(simgrid automaton_generated_src)
endif()
