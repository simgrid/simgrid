### Make Libs
#>gcc c:\simgrid-trunk\examples\msg\icomms\peer.c -static -Lc:\simgrid-trunk\lib -lsimgrid -Ic:\simgrid-trunk\include -lwsock32	
add_library(simgrid STATIC ${simgrid_sources})
add_library(gras STATIC ${gras_sources})

if(MSVC)
    set_target_properties(gras     PROPERTIES COMPILE_FLAGS "/D _XBT_DLL_STATIC"
                                                     OUTPUT_NAME   "gras")
    set_target_properties(simgrid  PROPERTIES COMPILE_FLAGS "/D _XBT_DLL_STATIC"
                                                     OUTPUT_NAME   "simgrid")
else(MSVC)
    if(CMAKE_COMPILER_IS_GNUCC)
        set_target_properties(gras     PROPERTIES COMPILE_FLAGS "-D_XBT_DLL_STATIC"
                                                         OUTPUT_NAME   "gras")
        set_target_properties(simgrid  PROPERTIES COMPILE_FLAGS "-D_XBT_DLL_STATIC"
                                                         OUTPUT_NAME   "simgrid")
    else(CMAKE_COMPILER_IS_GNUCC)
        message(FATAL_ERROR "Compilateur non connu!!!")
    endif(CMAKE_COMPILER_IS_GNUCC)
endif(MSVC)

set(GRAS_DEP "wsock32")
set(SIMGRID_DEP "wsock32")

target_link_libraries(simgrid 	${SIMGRID_DEP})
target_link_libraries(gras 	${GRAS_DEP})

### Make EXEs

#src/testall
add_subdirectory(${PROJECT_DIRECTORY}/src)

#tools/gras
add_subdirectory(${PROJECT_DIRECTORY}/tools/gras)

#tools/tesh
#add_subdirectory(${PROJECT_DIRECTORY}/tools/tesh)

#testsuite/xbt
add_subdirectory(${PROJECT_DIRECTORY}/testsuite/xbt)

#testsuite/surf
add_subdirectory(${PROJECT_DIRECTORY}/testsuite/surf)

#testsuite/simdag
add_subdirectory(${PROJECT_DIRECTORY}/testsuite/simdag)

#teshsuite
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/xbt)
#add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/gras/datadesc)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/gras/msg_handle)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/gras/empty_main)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/gras/small_sleep)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/simdag)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/simdag/network)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/simdag/network/p2p)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/simdag/network/mxn)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/simdag/partask)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/simdag/platforms)
add_subdirectory(${PROJECT_DIRECTORY}/teshsuite/msg)

#examples
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/ping)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/rpc)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/spawn)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/timer)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/chrono)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/mutual_exclusion/simple_token)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/mmrpc)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/all2all)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/pmm)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/synchro)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/properties)
add_subdirectory(${PROJECT_DIRECTORY}/examples/gras/console)

add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/properties)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/actions)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/migration)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/sendrecv)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/suspend)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/parallel_task)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/priority)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/masterslave)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/trace)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/tracing)
add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/icomms)

if(HAVE_MC)
	add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/mc)
endif(HAVE_MC)

if(HAVE_GTNETS)
	add_definitions("-lgtnets -L${gtnets_path}/lib -I${gtnets_path}/include/gtnets")
	add_subdirectory(${PROJECT_DIRECTORY}/examples/msg/gtnets)
endif(HAVE_GTNETS)

add_subdirectory(${PROJECT_DIRECTORY}/examples/amok/bandwidth)
add_subdirectory(${PROJECT_DIRECTORY}/examples/amok/saturate)

add_subdirectory(${PROJECT_DIRECTORY}/examples/simdag)
add_subdirectory(${PROJECT_DIRECTORY}/examples/simdag/dax)
if(enable_graphviz)
add_subdirectory(${PROJECT_DIRECTORY}/examples/simdag/dot)
endif(enable_graphviz)
add_subdirectory(${PROJECT_DIRECTORY}/examples/simdag/metaxml)
add_subdirectory(${PROJECT_DIRECTORY}/examples/simdag/properties)
add_subdirectory(${PROJECT_DIRECTORY}/examples/simdag/scheduling)
if(enable_smpi)
	add_subdirectory(${PROJECT_DIRECTORY}/examples/smpi)
endif(enable_smpi)