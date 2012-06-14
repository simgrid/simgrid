###################################################################
### Load all files declaring binaries (tools, examples and tests) #
###################################################################
##################################################################
# Those CMakelists or just here to define extra files in dist    #
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/lua)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/xbt)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/gras)
##################################################################

add_subdirectory(${CMAKE_HOME_DIRECTORY}/tools/gras)

if(WIN32)
	add_custom_target(tesh ALL
	DEPENDS ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/Scripts/tesh.pl
	COMMENT "Install ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/Scripts/tesh.pl"
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_HOME_DIRECTORY}/buildtools/Cmake/Scripts/tesh.pl ${CMAKE_BINARY_DIR}/bin/tesh
	)
else(WIN32)
	add_subdirectory(${CMAKE_HOME_DIRECTORY}/tools/tesh)
endif(WIN32)

add_subdirectory(${CMAKE_HOME_DIRECTORY}/tools/graphicator/)

add_subdirectory(${CMAKE_HOME_DIRECTORY}/testsuite/xbt)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/testsuite/surf)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/xbt)
if(NOT WIN32)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/gras/datadesc)
endif(NOT WIN32)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/gras/msg_handle)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/gras/empty_main)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/gras/small_sleep)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/simdag)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/network)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/network/p2p)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/network/mxn)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/partask)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/simdag/platforms)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/msg)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/teshsuite/msg/trace)

add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/ping)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/rpc)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/spawn)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/timer)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/chrono)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/mutual_exclusion/simple_token)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/mmrpc)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/all2all)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/pmm)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/synchro)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/properties)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/gras/console)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/properties)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/actions)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/migration)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/sendrecv)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/suspend)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/parallel_task)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/priority)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/masterslave)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/icomms)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/chord)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/token_ring)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/pmm)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/start_kill_time)

add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/io)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/cloud)

add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/gpu)

if(HAVE_TRACING)
    add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/tracing)
endif(HAVE_TRACING)

if(HAVE_MC)
	add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/mc)
endif(HAVE_MC)

add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/gtnets)

if(HAVE_NS3)
	add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/msg/ns3)
endif(HAVE_NS3)

add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/amok/bandwidth)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/amok/saturate)

add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/simdag)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/simdag/dax)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/simdag/goal)
if(HAVE_GRAPHVIZ)
  add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/simdag/dot)
endif(HAVE_GRAPHVIZ)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/simdag/metaxml)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/simdag/properties)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/simdag/scheduling)

if(enable_smpi)
	add_subdirectory(${CMAKE_HOME_DIRECTORY}/examples/smpi)
endif(enable_smpi)
