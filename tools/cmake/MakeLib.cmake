### Make Libs

# On Mac OSX, don't specify where to look for the dependencies
# Otherwise, the libsimgrid-java will not find libsimgrid, as it is located in a temporary directory
set(MACOSX_RPATH 0)

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

if(HAVE_PTHREAD AND ${HAVE_THREAD_CONTEXTS} AND NOT APPLE)
  # Clang on recent Mac OS X is not happy about -pthread.
  SET(SIMGRID_DEP "${SIMGRID_DEP} -pthread")
endif()

if(HAVE_LUA)
  ADD_CUSTOM_TARGET(link_simgrid_lua ALL
    DEPENDS 	simgrid
    ${CMAKE_BINARY_DIR}/examples/lua/simgrid.${LIB_EXE}
    ${CMAKE_BINARY_DIR}/examples/msg/masterslave/simgrid.${LIB_EXE}
    ${CMAKE_BINARY_DIR}/examples/simdag/simgrid.${LIB_EXE}
    )
  add_custom_command(
    OUTPUT 	${CMAKE_BINARY_DIR}/examples/lua/simgrid.${LIB_EXE}
    ${CMAKE_BINARY_DIR}/examples/msg/masterslave/simgrid.${LIB_EXE}
    ${CMAKE_BINARY_DIR}/examples/simdag/simgrid.${LIB_EXE}
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/examples/lua/simgrid.${LIB_EXE} # if it exists, creating the link fails. So cleanup before hand
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/examples/lua/
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/lib/libsimgrid.${LIB_EXE} ${CMAKE_BINARY_DIR}/examples/lua/simgrid.${LIB_EXE} #for test

    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/examples/msg/masterslave/simgrid.${LIB_EXE} # if it exists, creating the link fails. So cleanup before hand
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/examples/msg/masterslave/
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/lib/libsimgrid.${LIB_EXE} ${CMAKE_BINARY_DIR}/examples/msg/masterslave/simgrid.${LIB_EXE} #for test

    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/examples/simdag/simgrid.${LIB_EXE} # if it exists, creating the link fails. So cleanup before hand
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/examples/simdag/
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/lib/libsimgrid.${LIB_EXE} ${CMAKE_BINARY_DIR}/examples/simdag/simgrid.${LIB_EXE} #for test
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

if(HAVE_MC AND HAVE_GNU_LD)
  SET(SIMGRID_DEP "${SIMGRID_DEP} ${DL_LIBRARY}")
endif()

if(HAVE_NS3)
  SET(SIMGRID_DEP "${SIMGRID_DEP} -lns${NS3_VERSION}-core${NS3_SUFFIX} -lns${NS3_VERSION}-csma${NS3_SUFFIX} -lns${NS3_VERSION}-point-to-point${NS3_SUFFIX} -lns${NS3_VERSION}-internet${NS3_SUFFIX} -lns${NS3_VERSION}-applications${NS3_SUFFIX}")
endif()

if(HAVE_POSIX_GETTIME)
  SET(SIMGRID_DEP "${SIMGRID_DEP} -lrt")
endif()

if("${CMAKE_SYSTEM}" MATCHES "FreeBSD")
  set(SIMGRID_DEP "${SIMGRID_DEP} -lprocstat")
endif()

# Compute the dependencies of SMPI
##################################
if(enable_smpi AND APPLE)
  set(SIMGRID_DEP "${SIMGRID_DEP} -Wl,-U -Wl,_smpi_simulated_main")
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
